import express from 'express';
import cors from 'cors';
import { execFile } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { v4 as uuidv4 } from 'uuid';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');
const RESUMEC_BIN = path.join(PROJECT_ROOT, 'bin', 'resumec');
const OUTPUTS_DIR = path.join(PROJECT_ROOT, 'outputs');

// Ensure outputs directory exists
if (!fs.existsSync(OUTPUTS_DIR)) {
  fs.mkdirSync(OUTPUTS_DIR, { recursive: true });
}

const app = express();
app.use(cors());
app.use(express.json({ limit: '1mb' }));

// Serve generated PDFs statically
app.use('/outputs', express.static(OUTPUTS_DIR));

/**
 * Escape a string for use in the .resume DSL format.
 * Wraps in double-quotes and escapes embedded quotes and backslashes.
 */
function escapeDslString(str) {
  return '"' + str.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '"';
}

/**
 * Convert form data to .resume DSL file content
 */
function buildDslContent(data) {
  const lines = [];

  // Required: name
  lines.push(`name ${escapeDslString(data.name)}`);

  // Optional contact info
  if (data.phone) lines.push(`phone ${escapeDslString(data.phone)}`);
  if (data.email) lines.push(`email ${escapeDslString(data.email)}`);
  if (data.github) lines.push(`github ${escapeDslString(data.github)}`);
  if (data.linkedin) lines.push(`linkedin ${escapeDslString(data.linkedin)}`);

  // Education section
  if (data.education && data.education.length > 0) {
    lines.push('');
    lines.push('education');
    for (const edu of data.education) {
      lines.push(`college ${escapeDslString(edu.college)}`);
      lines.push(`cgpa ${parseFloat(edu.cgpa).toFixed(2)}`);
    }
  }

  // Skills section
  if (data.skills && data.skills.length > 0) {
    lines.push('');
    lines.push('skills');
    for (const skill of data.skills) {
      // Skills can be bare identifiers in the DSL
      lines.push(skill.trim());
    }
  }

  // Projects section
  if (data.projects && data.projects.length > 0) {
    lines.push('');
    lines.push('projects');
    for (const proj of data.projects) {
      lines.push(`project ${escapeDslString(proj.title)}`);
      for (const bullet of (proj.bullets || [])) {
        if (bullet.trim()) {
          lines.push(`bullet ${escapeDslString(bullet)}`);
        }
      }
    }
  }

  // Experience section
  if (data.experience && data.experience.length > 0) {
    lines.push('');
    lines.push('experience');
    for (const exp of data.experience) {
      lines.push(`company ${escapeDslString(exp.company)}`);
      if (exp.role) lines.push(`role ${escapeDslString(exp.role)}`);
      if (exp.duration) lines.push(`duration ${escapeDslString(exp.duration)}`);
      for (const bullet of (exp.bullets || [])) {
        if (bullet.trim()) {
          lines.push(`bullet ${escapeDslString(bullet)}`);
        }
      }
    }
  }

  // Custom/extra sections
  if (data.extras && data.extras.length > 0) {
    for (const extra of data.extras) {
      lines.push('');
      lines.push(`extra ${escapeDslString(extra.title)}`);
      for (const item of (extra.items || [])) {
        if (item.text && item.text.trim()) {
          lines.push(`item ${escapeDslString(item.text)}`);
          for (const sub of (item.subbullets || [])) {
            if (sub.trim()) {
              lines.push(`bullet ${escapeDslString(sub)}`);
            }
          }
        }
      }
    }
  }

  return lines.join('\n') + '\n';
}

/**
 * Run the resumec binary via WSL (since it's a Linux binary compiled under WSL).
 * On Linux/Mac, runs directly.
 */
function runResumec(resumePath, pdfPath, cgpaScale) {
  return new Promise((resolve, reject) => {
    // Convert Windows paths to WSL paths
    const wslResumePath = resumePath.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => `/mnt/${d.toLowerCase()}`);
    const wslPdfPath = pdfPath.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => `/mnt/${d.toLowerCase()}`);
    const wslBin = RESUMEC_BIN.replace(/\\/g, '/').replace(/^([A-Za-z]):/, (_, d) => `/mnt/${d.toLowerCase()}`);

    const args = [
      '-d', 'Ubuntu', 'bash', '-c',
      `"${wslBin}" "${wslResumePath}" -o "${wslPdfPath}" --cgpa-scale ${cgpaScale}`
    ];

    execFile('wsl', args, { timeout: 60000 }, (error, stdout, stderr) => {
      const output = (stdout + stderr).trim();
      if (error) {
        reject(new Error(output || error.message));
      } else {
        resolve(output);
      }
    });
  });
}

/**
 * POST /api/generate
 * Body: resume form data as JSON
 * Returns: { success, pdfUrl, pdfPath, dslContent, message }
 */
app.post('/api/generate', async (req, res) => {
  const data = req.body;

  // Basic validation
  if (!data.name || !data.name.trim()) {
    return res.status(400).json({ success: false, error: 'Name is required.' });
  }

  const jobId = uuidv4();
  const safeBase = data.name.toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/^_|_$/g, '') || 'resume';
  const resumeFilename = `${safeBase}_${jobId.slice(0, 8)}.resume`;
  const pdfFilename = `${safeBase}_${jobId.slice(0, 8)}.pdf`;

  const resumePath = path.join(OUTPUTS_DIR, resumeFilename);
  const pdfPath = path.join(OUTPUTS_DIR, pdfFilename);

  const cgpaScale = data.cgpaScale || 10;

  try {
    // Build DSL content
    const dslContent = buildDslContent(data);

    // Write .resume DSL file
    fs.writeFileSync(resumePath, dslContent, 'utf8');

    // Run the compiler
    const output = await runResumec(resumePath, pdfPath, cgpaScale);

    // Check if PDF was actually created
    if (!fs.existsSync(pdfPath)) {
      return res.status(500).json({
        success: false,
        error: 'PDF was not generated. Compiler output: ' + output,
        dslContent,
      });
    }

    res.json({
      success: true,
      pdfUrl: `/outputs/${pdfFilename}`,
      pdfFilename,
      resumeFilename,
      dslContent,
      message: output,
    });
  } catch (err) {
    // Clean up on error
    try { fs.unlinkSync(resumePath); } catch (_) {}
    try { fs.unlinkSync(pdfPath); } catch (_) {}

    res.status(500).json({
      success: false,
      error: err.message || 'Compilation failed.',
    });
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
  });
});

const PORT = process.env.PORT || 3001;
app.listen(PORT, () => {
  console.log(`resumec server running on http://localhost:${PORT}`);
  console.log(`Binary: ${RESUMEC_BIN}`);
  console.log(`Outputs: ${OUTPUTS_DIR}`);
});
