import express from 'express';
import cors from 'cors';
import { execFile } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { v4 as uuidv4 } from 'uuid';
import multer from 'multer';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');
const RESUMEC_BIN = path.join(PROJECT_ROOT, 'bin', 'resumec');
const OUTPUTS_DIR = path.join(PROJECT_ROOT, 'outputs');
const UPLOADS_DIR = path.join(OUTPUTS_DIR, 'uploads');

// Ensure output directories exist
if (!fs.existsSync(OUTPUTS_DIR)) {
  fs.mkdirSync(OUTPUTS_DIR, { recursive: true });
}
if (!fs.existsSync(UPLOADS_DIR)) {
  fs.mkdirSync(UPLOADS_DIR, { recursive: true });
}

const upload = multer({ dest: UPLOADS_DIR, limits: { fileSize: 15 * 1024 * 1024 } });

const app = express();
app.use(cors());
app.use(express.json({ limit: '5mb' }));

// Serve generated outputs (PDFs, TEX, DSL) statically
app.use('/outputs', express.static(OUTPUTS_DIR));

// Request logger — every request is printed so you can see backend activity
app.use((req, _res, next) => {
  console.log(`[${new Date().toLocaleTimeString()}] ${req.method} ${req.path}`);
  next();
});

/**
 * Escape a string for use in the .resume DSL format.
 */
function escapeDslString(str) {
  if (!str) return '""';
  return '"' + str.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '"';
}

function formatSkillDsl(skill) {
  const s = skill.trim();
  if (/^[A-Za-z][A-Za-z0-9_.+#-]*$/.test(s)) {
    return s;
  }
  return escapeDslString(s);
}

/**
 * Convert structured resume form data to clean .resume DSL syntax.
 */
function buildDslContent(data) {
  const lines = [];

  // Required: name
  lines.push(`name ${escapeDslString(data.name || 'Anonymous')}`);

  // Contact info
  if (data.phone) lines.push(`phone ${escapeDslString(data.phone)}`);
  if (data.email) lines.push(`email ${escapeDslString(data.email)}`);
  if (data.github) lines.push(`github ${escapeDslString(data.github)}`);
  if (data.linkedin) lines.push(`linkedin ${escapeDslString(data.linkedin)}`);

  // Education section
  if (data.education && data.education.length > 0) {
    lines.push('');
    lines.push('education');
    for (const edu of data.education) {
      if (edu.college && edu.college.trim()) {
        lines.push(`college ${escapeDslString(edu.college)}`);
        const cgpaVal = parseFloat(edu.cgpa);
        lines.push(`cgpa ${isNaN(cgpaVal) ? '9.00' : cgpaVal.toFixed(2)}`);
      }
    }
  }

  // Skills section
  if (data.skills && data.skills.length > 0) {
    lines.push('');
    lines.push('skills');
    for (const skill of data.skills) {
      if (skill && skill.trim()) {
        lines.push(formatSkillDsl(skill));
      }
    }
  }

  // Projects section
  if (data.projects && data.projects.length > 0) {
    lines.push('');
    lines.push('projects');
    for (const proj of data.projects) {
      const title = proj.title || proj.name;
      if (title && title.trim()) {
        lines.push(`project ${escapeDslString(title)}`);
        for (const bullet of (proj.bullets || [])) {
          if (bullet && bullet.trim()) {
            lines.push(`bullet ${escapeDslString(bullet)}`);
          }
        }
      }
    }
  }

  // Experience section
  if (data.experience && data.experience.length > 0) {
    lines.push('');
    lines.push('experience');
    for (const exp of data.experience) {
      if (exp.company && exp.company.trim()) {
        lines.push(`company ${escapeDslString(exp.company)}`);
        if (exp.role) lines.push(`role ${escapeDslString(exp.role)}`);
        if (exp.duration) lines.push(`duration ${escapeDslString(exp.duration)}`);
        for (const bullet of (exp.bullets || [])) {
          if (bullet && bullet.trim()) {
            lines.push(`bullet ${escapeDslString(bullet)}`);
          }
        }
      }
    }
  }

  // Custom/extra sections
  if (data.extras && data.extras.length > 0) {
    for (const extra of data.extras) {
      if (extra.title && extra.title.trim()) {
        lines.push('');
        lines.push(`extra ${escapeDslString(extra.title)}`);
        for (const item of (extra.items || [])) {
          if (item.text && item.text.trim()) {
            lines.push(`item ${escapeDslString(item.text)}`);
            for (const sub of (item.subbullets || [])) {
              if (sub && sub.trim()) {
                lines.push(`bullet ${escapeDslString(sub)}`);
              }
            }
          }
        }
      }
    }
  }

  return lines.join('\n') + '\n';
}

/**
 * Run the resumec binary directly on macOS/Linux, or through WSL on Windows.
 */
function runResumecCmd(binArgs) {
  return new Promise((resolve, reject) => {
    if (process.platform === 'win32') {
      const wslBin = RESUMEC_BIN.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => `/mnt/${d.toLowerCase()}`);
      const wslArgs = binArgs.map(arg => {
        if (typeof arg === 'string' && /^[A-Za-z]:/.test(arg)) {
          return arg.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => `/mnt/${d.toLowerCase()}`);
        }
        return arg;
      });
      const cmdStr = `"${wslBin}" ` + wslArgs.map(a => `"${a}"`).join(' ');
      execFile('wsl', ['-d', 'Ubuntu', 'bash', '-c', cmdStr], { timeout: 60000 }, (error, stdout, stderr) => {
        const out = stdout.trim();
        const err = stderr.trim();
        if (error) {
          reject(new Error(err || out || error.message));
        } else {
          resolve(out || err);
        }
      });
    } else {
      execFile(RESUMEC_BIN, binArgs, { timeout: 60000 }, (error, stdout, stderr) => {
        const out = stdout.trim();
        const err = stderr.trim();
        if (error) {
          reject(new Error(err || out || error.message));
        } else {
          resolve(out || err);
        }
      });
    }
  });
}

/**
 * Parse job analysis text from resumec stdout if present.
 */
function parseJobAnalysisFromOutput(output) {
  if (!output || !output.includes('JOB REQUIREMENTS MATCH ANALYSIS')) return null;

  const scoreMatch = output.match(/Match Score:\s*\[.*?\]\s*([0-9.]+)%/);
  const matchScore = scoreMatch ? parseFloat(scoreMatch[1]) : 0;

  const matchedSkills = [];
  const matchedPart = output.match(/Matched Skills in Resume.*?:([\s\S]*?)(?:⚠️|┗|$)/);
  if (matchedPart) {
    const raw = matchedPart[1];
    const items = raw.match(/[•*]\s*([^,\n•*]+)/g);
    if (items) {
      items.forEach(it => {
        const s = it.replace(/^[•*]\s*/, '').trim();
        if (s && !s.includes('No required skills')) matchedSkills.push(s);
      });
    }
  }

  const missingSkills = [];
  const missingPart = output.match(/Missing Skills Mentioned in Job.*?:([\s\S]*?)(?:┗|$)/);
  if (missingPart) {
    const raw = missingPart[1];
    const items = raw.match(/[✕x*]\s*([^,\n✕x*]+)/g);
    if (items) {
      items.forEach(it => {
        const s = it.replace(/^[✕x*]\s*/, '').trim();
        if (s && !s.includes('Perfect match')) missingSkills.push(s);
      });
    }
  }

  return {
    matchPercentage: matchScore,
    matchedCount: matchedSkills.length,
    matchedSkills,
    missingCount: missingSkills.length,
    missingSkills,
  };
}

/**
 * Parse diagnostics from compiler output.
 */
function parseDiagnostics(output) {
  const diags = [];
  const lines = output.split('\n');
  for (const l of lines) {
    const m = l.match(/error\s*\[line\s*(\d+),\s*col\s*(\d+)\]:\s*(.*)/i);
    if (m) {
      diags.push({
        line: parseInt(m[1], 10),
        col: parseInt(m[2], 10),
        message: m[3].trim(),
      });
    }
  }
  return diags;
}

/**
 * POST /api/generate
 * Compile structured resume form data to PDF with optional job analysis.
 */
app.post('/api/generate', async (req, res) => {
  const data = req.body;

  if (!data.name || !data.name.trim()) {
    return res.status(400).json({ success: false, error: 'Name is required.' });
  }

  const jobId = uuidv4();
  const safeBase = data.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '') || 'resume';
  const resumeFilename = `${safeBase}_${jobId.slice(0, 8)}.resume`;
  const pdfFilename = `${safeBase}_${jobId.slice(0, 8)}.pdf`;
  const texFilename = `${safeBase}_${jobId.slice(0, 8)}.tex`;

  const resumePath = path.join(OUTPUTS_DIR, resumeFilename);
  const pdfPath = path.join(OUTPUTS_DIR, pdfFilename);
  const texPath = path.join(OUTPUTS_DIR, texFilename);

  const cgpaScale = data.cgpaScale || 10;
  let tempJobFile = null;

  try {
    const dslContent = buildDslContent(data);
    fs.writeFileSync(resumePath, dslContent, 'utf8');

    const args = [resumePath, '-o', pdfPath, '--cgpa-scale', String(cgpaScale), '--keep-tex'];

    if (data.jobUrl && data.jobUrl.trim()) {
      args.push('--job-url', data.jobUrl.trim());
    } else if (data.jobDesc && data.jobDesc.trim()) {
      tempJobFile = path.join(OUTPUTS_DIR, `job_${jobId.slice(0, 8)}.txt`);
      fs.writeFileSync(tempJobFile, data.jobDesc.trim(), 'utf8');
      args.push('--job-desc', tempJobFile);
    }

    const output = await runResumecCmd(args);

    if (!fs.existsSync(pdfPath)) {
      const diags = parseDiagnostics(output);
      return res.status(400).json({
        success: false,
        error: output || 'PDF compilation failed.',
        diagnostics: diags,
        dslContent,
      });
    }

    let texContent = '';
    if (fs.existsSync(texPath)) {
      texContent = fs.readFileSync(texPath, 'utf8');
    }

    const jobAnalysis = parseJobAnalysisFromOutput(output);

    res.json({
      success: true,
      pdfUrl: `/outputs/${pdfFilename}`,
      pdfFilename,
      resumeFilename,
      dslContent,
      texContent,
      jobAnalysis,
      message: output,
    });
  } catch (err) {
    res.status(500).json({
      success: false,
      error: err.message || 'Compilation failed.',
      diagnostics: parseDiagnostics(err.message || ''),
    });
  } finally {
    if (tempJobFile && fs.existsSync(tempJobFile)) {
      try { fs.unlinkSync(tempJobFile); } catch (_) {}
    }
  }
});

/**
 * POST /api/compile-dsl
 * Compile raw .resume DSL text directly (for code playground / live editor).
 */
app.post('/api/compile-dsl', async (req, res) => {
  const { dslContent, jobUrl, jobDesc, cgpaScale } = req.body;

  if (!dslContent || !dslContent.trim()) {
    return res.status(400).json({ success: false, error: 'DSL content is required.' });
  }

  const jobId = uuidv4();
  const resumeFilename = `custom_${jobId.slice(0, 8)}.resume`;
  const pdfFilename = `custom_${jobId.slice(0, 8)}.pdf`;
  const texFilename = `custom_${jobId.slice(0, 8)}.tex`;

  const resumePath = path.join(OUTPUTS_DIR, resumeFilename);
  const pdfPath = path.join(OUTPUTS_DIR, pdfFilename);
  const texPath = path.join(OUTPUTS_DIR, texFilename);

  let tempJobFile = null;

  try {
    fs.writeFileSync(resumePath, dslContent, 'utf8');

    const args = [resumePath, '-o', pdfPath, '--cgpa-scale', String(cgpaScale || 10), '--keep-tex'];

    if (jobUrl && jobUrl.trim()) {
      args.push('--job-url', jobUrl.trim());
    } else if (jobDesc && jobDesc.trim()) {
      tempJobFile = path.join(OUTPUTS_DIR, `job_${jobId.slice(0, 8)}.txt`);
      fs.writeFileSync(tempJobFile, jobDesc.trim(), 'utf8');
      args.push('--job-desc', tempJobFile);
    }

    const output = await runResumecCmd(args);

    if (!fs.existsSync(pdfPath)) {
      const diags = parseDiagnostics(output);
      return res.status(400).json({
        success: false,
        error: output || 'PDF compilation failed.',
        diagnostics: diags,
      });
    }

    let texContent = '';
    if (fs.existsSync(texPath)) {
      texContent = fs.readFileSync(texPath, 'utf8');
    }

    const jobAnalysis = parseJobAnalysisFromOutput(output);

    res.json({
      success: true,
      pdfUrl: `/outputs/${pdfFilename}`,
      pdfFilename,
      resumeFilename,
      dslContent,
      texContent,
      jobAnalysis,
      message: output,
    });
  } catch (err) {
    res.status(400).json({
      success: false,
      error: err.message || 'Compilation failed.',
      diagnostics: parseDiagnostics(err.message || ''),
    });
  } finally {
    if (tempJobFile && fs.existsSync(tempJobFile)) {
      try { fs.unlinkSync(tempJobFile); } catch (_) {}
    }
  }
});

/**
 * POST /api/analyze-job
 * Live scraper and requirement matcher for target job URL or text.
 */
app.post('/api/analyze-job', async (req, res) => {
  const { jobUrl, jobDesc, currentSkills } = req.body;
  const target = (jobUrl && jobUrl.trim()) ? jobUrl.trim() : null;

  let tempJobFile = null;
  const targetPath = target || (jobDesc && jobDesc.trim() ? (() => {
    const p = path.join(OUTPUTS_DIR, `job_probe_${uuidv4().slice(0, 8)}.txt`);
    fs.writeFileSync(p, jobDesc.trim(), 'utf8');
    tempJobFile = p;
    return p;
  })() : null);

  if (!targetPath) {
    return res.status(400).json({ success: false, error: 'Target job URL or description is required.' });
  }

  try {
    const output = await runResumecCmd(['--analyze-job', targetPath, '--json']);
    const parsed = JSON.parse(output);

    const jobAnalysis = parsed.jobAnalysis || {};
    const jobSkills = jobAnalysis.jobSkills || [];

    // If currentSkills were passed from the frontend, compute match against them:
    if (Array.isArray(currentSkills) && currentSkills.length > 0) {
      const skillsNorm = currentSkills.map(s => s.toLowerCase().trim());
      const matched = [];
      const missing = [];

      jobSkills.forEach(js => {
        if (skillsNorm.some(cs => cs === js.toLowerCase() || js.toLowerCase().includes(cs) || cs.includes(js.toLowerCase()))) {
          matched.push(js);
        } else {
          missing.push(js);
        }
      });

      const total = jobSkills.length;
      const score = total > 0 ? (matched.length / total) * 100 : 100;

      jobAnalysis.matchedSkills = matched;
      jobAnalysis.matchedCount = matched.length;
      jobAnalysis.missingSkills = missing;
      jobAnalysis.missingCount = missing.length;
      jobAnalysis.matchPercentage = Math.round(score * 10) / 10;
    }

    res.json({
      success: true,
      jobAnalysis,
    });
  } catch (err) {
    res.status(500).json({
      success: false,
      error: err.message || 'Job analysis failed.',
    });
  } finally {
    if (tempJobFile && fs.existsSync(tempJobFile)) {
      try { fs.unlinkSync(tempJobFile); } catch (_) {}
    }
  }
});

/**
 * POST /api/upload-resume
 * Upload existing resume PDF or .resume DSL file and extract all data + optional job gap analysis.
 */
app.post('/api/upload-resume', upload.any(), async (req, res) => {
  // With upload.any(), files are in req.files array — find the resume file
  const resumeFile = (req.files || []).find(f => f.fieldname === 'resumeFile') || (req.files || [])[0];
  if (!resumeFile) {
    return res.status(400).json({ success: false, error: 'No resume file uploaded.' });
  }

  const uploadedPath = resumeFile.path;
  const originalName = resumeFile.originalname || 'uploaded_resume';
  const ext = path.extname(originalName).toLowerCase();

  // Create appropriately named file for resumec
  const namedFile = path.join(UPLOADS_DIR, `upload_${uuidv4().slice(0, 8)}${ext || '.pdf'}`);
  fs.renameSync(uploadedPath, namedFile);

  const { jobUrl, jobDesc } = req.body;
  let tempJobFile = null;

  try {
    const args = ['--extract-json', namedFile, '--json'];

    if (jobUrl && jobUrl.trim()) {
      args.push('--job-url', jobUrl.trim());
    } else if (jobDesc && jobDesc.trim()) {
      tempJobFile = path.join(OUTPUTS_DIR, `job_${uuidv4().slice(0, 8)}.txt`);
      fs.writeFileSync(tempJobFile, jobDesc.trim(), 'utf8');
      args.push('--job-desc', tempJobFile);
    }

    const output = await runResumecCmd(args);
    const parsed = JSON.parse(output);

    // Build synthesized DSL for easy preview/editing
    let dslContent = '';
    if (parsed.resume) {
      dslContent = buildDslContent(parsed.resume);
    }

    res.json({
      success: true,
      resume: parsed.resume || {},
      jobAnalysis: parsed.jobAnalysis || null,
      dslContent,
    });
  } catch (err) {
    res.status(500).json({
      success: false,
      error: err.message || 'Failed to extract resume from uploaded file.',
    });
  } finally {
    if (tempJobFile && fs.existsSync(tempJobFile)) {
      try { fs.unlinkSync(tempJobFile); } catch (_) {}
    }
    // Clean up uploaded file
    try { fs.unlinkSync(namedFile); } catch (_) {}
  }
});

/**
 * POST /api/ast
 * Dump AST for DSL or resume structure.
 */
app.post('/api/ast', async (req, res) => {
  const { dslContent } = req.body;
  if (!dslContent) {
    return res.status(400).json({ success: false, error: 'DSL content required' });
  }

  const tmpDsl = path.join(OUTPUTS_DIR, `ast_${uuidv4().slice(0, 8)}.resume`);
  try {
    fs.writeFileSync(tmpDsl, dslContent, 'utf8');
    const output = await runResumecCmd([tmpDsl, '--dump-ast', '--json']);
    const parsed = JSON.parse(output);
    res.json({ success: true, ast: parsed.resume || parsed });
  } catch (err) {
    res.status(400).json({ success: false, error: err.message });
  } finally {
    try { fs.unlinkSync(tmpDsl); } catch (_) {}
  }
});

/**
 * GET /api/health
 */
app.get('/api/health', (req, res) => {
  const binExists = fs.existsSync(RESUMEC_BIN);
  res.json({
    status: 'ok',
    binExists,
    binPath: RESUMEC_BIN,
    platform: process.platform,
    version: 'resumec 1.0 (C11/Bison/Flex/pdflatex)',
  });
});


// 404 catch-all for unknown /api/* routes — prevents Vite proxy ECONNREFUSED noise
app.use('/api', (req, res) => {
  res.status(404).json({ success: false, error: `Unknown endpoint: ${req.method} ${req.path}` });
});

// Multer error handler — catches "Unexpected field" and file-size errors
// eslint-disable-next-line no-unused-vars
app.use((err, req, res, _next) => {
  if (err && err.code === 'LIMIT_UNEXPECTED_FILE') {
    return res.status(400).json({ success: false, error: `Unexpected file field: ${err.field}` });
  }
  if (err && err.code === 'LIMIT_FILE_SIZE') {
    return res.status(413).json({ success: false, error: 'File too large (max 15 MB).' });
  }
  console.error('[server error]', err);
  res.status(500).json({ success: false, error: err.message || 'Internal server error' });
});


const PORT = process.env.PORT || 3001;
app.listen(PORT, () => {
  console.log(`resumec server running on http://localhost:${PORT}`);
  console.log(`Binary: ${RESUMEC_BIN}`);
  console.log(`Outputs: ${OUTPUTS_DIR}`);
});
