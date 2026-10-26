import { useState, useRef, useCallback, useEffect } from 'react'

// ─── Icons ──────────────────────────────────────────────────
const Icon = ({ name, size = 16 }) => {
  const icons = {
    plus:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="8" y1="2" x2="8" y2="14"/><line x1="2" y1="8" x2="14" y2="8"/></svg>,
    trash:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="2,4 14,4"/><path d="M5,4V2h6v2"/><rect x="3" y="4" width="10" height="10" rx="1"/><line x1="6" y1="7" x2="6" y2="11"/><line x1="10" y1="7" x2="10" y2="11"/></svg>,
    check:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="2,8 6,12 14,4"/></svg>,
    arrow_r:  <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="2" y1="8" x2="14" y2="8"/><polyline points="9,3 14,8 9,13"/></svg>,
    arrow_l:  <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="14" y1="8" x2="2" y2="8"/><polyline points="7,3 2,8 7,13"/></svg>,
    down:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="3,5 8,11 13,5"/></svg>,
    dl:       <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M8 2v8M5 7l3 4 3-4"/><path d="M2 12v2h12v-2"/></svg>,
    ext:      <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M9 2h5v5"/><line x1="14" y1="2" x2="7" y2="9"/><path d="M7 4H3a1 1 0 00-1 1v8a1 1 0 001 1h8a1 1 0 001-1V9"/></svg>,
    code:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="5,4 1,8 5,12"/><polyline points="11,4 15,8 11,12"/></svg>,
    alert:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M8 2L1 14h14L8 2z"/><line x1="8" y1="7" x2="8" y2="10"/><circle cx="8" cy="12.5" r="0.5" fill="currentColor"/></svg>,
    refresh:  <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M13.5 2.5A6.5 6.5 0 012 9"/><polyline points="11,0 13.5,2.5 11,5"/><path d="M2.5 13.5A6.5 6.5 0 0014 7"/><polyline points="5,11 2.5,13.5 5,16"/></svg>,
    doc:      <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><rect x="2" y="1" width="12" height="14" rx="1.5"/><line x1="5" y1="5" x2="11" y2="5"/><line x1="5" y1="8" x2="11" y2="8"/><line x1="5" y1="11" x2="9" y2="11"/></svg>,
    x:        <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="3" y1="3" x2="13" y2="13"/><line x1="13" y1="3" x2="3" y2="13"/></svg>,
    target:   <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><circle cx="8" cy="8" r="6"/><circle cx="8" cy="8" r="2.5"/></svg>,
    upload:   <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M8 11V2M4 6l4-4 4 4"/><path d="M2 13h12"/></svg>,
    copy:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><rect x="5" y="5" width="8" height="8" rx="1"/><path d="M3 11V3h8"/></svg>,
    tree:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><circle cx="8" cy="3" r="2"/><circle cx="4" cy="13" r="2"/><circle cx="12" cy="13" r="2"/><path d="M8 5v4M4 11l4-2 4 2"/></svg>,
    terminal: <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="3,5 6,8 3,11"/><line x1="8" y1="11" x2="13" y2="11"/></svg>,
  }
  return icons[name] || null
}

// ─── Validation helpers ──────────────────────────────────────
const PHONE_RE = /^[+]?[0-9 ()\-]{6,19}$/
const EMAIL_RE = /^[^@\s]+@[^@\s]+\.[^@\s]+$/
const GITHUB_RE = /^[A-Za-z0-9]([A-Za-z0-9-]{0,37}[A-Za-z0-9])?$/
const LINKEDIN_RE = /^[A-Za-z0-9_-]{3,100}$/

function countDigits(s) { return [...s].filter(c => c >= '0' && c <= '9').length }

function validatePersonal(data) {
  const errors = {}
  if (!data.name.trim()) errors.name = 'Full name is required.'
  if (data.phone && !PHONE_RE.test(data.phone)) errors.phone = 'Invalid phone number.'
  if (data.phone && (countDigits(data.phone) < 7 || countDigits(data.phone) > 15))
    errors.phone = 'Phone must have 7–15 digits.'
  if (data.email && !EMAIL_RE.test(data.email)) errors.email = 'Invalid email address.'
  if (data.github && !GITHUB_RE.test(data.github)) errors.github = 'Invalid GitHub username (no slashes).'
  if (data.linkedin && !LINKEDIN_RE.test(data.linkedin)) errors.linkedin = 'Invalid LinkedIn username.'
  return errors
}

function validateEducation(entries) {
  if (entries.length === 0) return {}
  const errors = {}
  entries.forEach((e, i) => {
    if (!e.college.trim()) errors[`edu_${i}_college`] = 'College name is required.'
    const cgpa = parseFloat(e.cgpa)
    if (isNaN(cgpa) || cgpa < 0) errors[`edu_${i}_cgpa`] = 'CGPA must be a positive number.'
  })
  return errors
}

function validateProjects(entries) {
  const errors = {}
  entries.forEach((e, i) => {
    const t = e.title || e.name || ''
    if (!t.trim()) errors[`proj_${i}_title`] = 'Project title is required.'
  })
  return errors
}

function validateExperience(entries) {
  const errors = {}
  entries.forEach((e, i) => {
    if (!e.company.trim()) errors[`exp_${i}_company`] = 'Company name is required.'
  })
  return errors
}

// ─── Reusable Field Components ───────────────────────────────
function Field({ label, optional, hint, error, children, className = '' }) {
  return (
    <div className={`field ${className}`}>
      {label && (
        <label className="field-label">
          {label}
          {optional && <span className="optional">(optional)</span>}
        </label>
      )}
      {hint && <span className="field-hint">{hint}</span>}
      {children}
      {error && <span className="field-error"><Icon name="alert" size={12}/> {error}</span>}
    </div>
  )
}

function Input({ className = '', ...props }) {
  return <input className={className} {...props} />
}

// ─── Skills Tag Input ────────────────────────────────────────
function SkillsInput({ skills, onChange }) {
  const [draft, setDraft] = useState('')
  const inputRef = useRef()

  const addSkill = useCallback((raw) => {
    const val = raw.trim()
    if (!val) return
    if (!skills.find(s => s.toLowerCase() === val.toLowerCase())) {
      onChange([...skills, val])
    }
    setDraft('')
  }, [skills, onChange])

  const handleKey = (e) => {
    if (e.key === 'Enter' || e.key === ',') {
      e.preventDefault()
      addSkill(draft)
    } else if (e.key === 'Backspace' && draft === '' && skills.length > 0) {
      onChange(skills.slice(0, -1))
    }
  }

  const handlePaste = (e) => {
    e.preventDefault()
    const text = e.clipboardData.getData('text')
    const parts = text.split(/[,\n]+/).map(s => s.trim()).filter(Boolean)
    const next = [...skills]
    parts.forEach(p => {
      if (!next.find(s => s.toLowerCase() === p.toLowerCase())) next.push(p)
    })
    onChange(next)
  }

  return (
    <div className="tags-input-area" onClick={() => inputRef.current?.focus()}>
      {skills.map((s, i) => (
        <span key={i} className="chip">
          {s}
          <button type="button" onClick={() => onChange(skills.filter((_, j) => j !== i))}>
            <Icon name="x" size={12}/>
          </button>
        </span>
      ))}
      <input
        ref={inputRef}
        value={draft}
        onChange={e => setDraft(e.target.value)}
        onKeyDown={handleKey}
        onPaste={handlePaste}
        onBlur={() => addSkill(draft)}
        placeholder={skills.length === 0 ? 'Type a skill and press Enter or comma…' : ''}
      />
    </div>
  )
}

// ─── Bullet list editor ──────────────────────────────────────
function BulletList({ bullets = [], onChange, placeholder = 'Add a bullet point…' }) {
  const addBullet = () => onChange([...bullets, ''])
  const update = (i, v) => onChange(bullets.map((b, j) => j === i ? v : b))
  const remove = (i) => onChange(bullets.filter((_, j) => j !== i))
  return (
    <div className="bullet-list">
      {bullets.map((b, i) => (
        <div className="bullet-row" key={i}>
          <span style={{ color: 'var(--text-faint)', fontSize: '0.8rem', paddingTop: 2 }}>•</span>
          <input
            value={b}
            onChange={e => update(i, e.target.value)}
            placeholder={placeholder}
          />
          <button className="btn btn-icon btn-danger" type="button" onClick={() => remove(i)}>
            <Icon name="trash" size={14}/>
          </button>
        </div>
      ))}
      <button className="btn btn-ghost btn-sm" type="button" onClick={addBullet}
        style={{ alignSelf: 'flex-start', gap: 4 }}>
        <Icon name="plus" size={13}/> Add bullet
      </button>
    </div>
  )
}

// ─── Target Job Application & Scraper Panel ──────────────────
function JobMatcherPanel({ jobUrl, setJobUrl, jobAnalysis, onAnalyze, onAddSkill, analyzing }) {
  const [open, setOpen] = useState(true)

  const matchScore = jobAnalysis ? jobAnalysis.matchPercentage : null
  const scoreClass = matchScore >= 75 ? 'high' : matchScore >= 45 ? 'med' : 'low'

  return (
    <div className={`job-matcher-panel ${jobAnalysis ? 'highlight' : ''}`}>
      <div className="job-matcher-header">
        <div className="job-matcher-title">
          <Icon name="target" size={18}/>
          <span>ATS Target Job Application Matcher</span>
          {matchScore !== null && (
            <span className={`match-badge ${scoreClass}`}>
              ★ {matchScore}% Match
            </span>
          )}
        </div>
        <button className="collapse-toggle" onClick={() => setOpen(v => !v)}>
          {open ? 'Hide' : 'Show'} <Icon name="down" size={12}/>
        </button>
      </div>

      {open && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.85rem' }}>
          <div style={{ display: 'flex', gap: '0.5rem', flexWrap: 'wrap' }}>
            <input
              style={{ flex: 1, minWidth: 260 }}
              value={jobUrl}
              onChange={e => setJobUrl(e.target.value)}
              placeholder="Paste job posting URL (e.g. Amazon, Google, LinkedIn) or file path…"
            />
            <button
              className="btn btn-primary btn-sm"
              onClick={onAnalyze}
              disabled={analyzing || !jobUrl.trim()}
            >
              {analyzing ? 'Scraping & Analyzing…' : 'Analyze Requirements'}
            </button>
          </div>

          {jobAnalysis && (
            <div style={{
              background: 'var(--bg)',
              borderRadius: 'var(--radius-sm)',
              padding: '0.85rem',
              border: '1px solid var(--border)'
            }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 6 }}>
                <span style={{ fontSize: '0.8rem', fontWeight: 600, color: 'var(--text-muted)' }}>
                  Coverage: {jobAnalysis.matchedCount} of {jobAnalysis.jobSkillCount || (jobAnalysis.matchedCount + jobAnalysis.missingCount)} requirements met
                </span>
                <span style={{ fontSize: '0.78rem', color: 'var(--text-faint)', fontFamily: 'var(--font-mono)' }}>
                  Source: {jobAnalysis.source || 'Job Link'}
                </span>
              </div>

              {jobAnalysis.matchedSkills && jobAnalysis.matchedSkills.length > 0 && (
                <div style={{ marginBottom: '0.5rem' }}>
                  <span style={{ fontSize: '0.75rem', fontWeight: 600, color: 'var(--success)' }}>
                    ✓ Already in your resume ({jobAnalysis.matchedSkills.length}):
                  </span>
                  <div className="skills-pills-wrap">
                    {jobAnalysis.matchedSkills.map((s, i) => (
                      <span key={i} className="skill-pill matched">
                        <Icon name="check" size={10}/> {s}
                      </span>
                    ))}
                  </div>
                </div>
              )}

              {jobAnalysis.missingSkills && jobAnalysis.missingSkills.length > 0 && (
                <div>
                  <span style={{ fontSize: '0.75rem', fontWeight: 600, color: 'var(--accent-dark)' }}>
                    ⚠️ Missing from job requirements ({jobAnalysis.missingSkills.length}) — Click + to add:
                  </span>
                  <div className="skills-pills-wrap">
                    {jobAnalysis.missingSkills.map((s, i) => (
                      <span key={i} className="skill-pill missing">
                        {s}
                        <button
                          type="button"
                          className="skill-add-btn"
                          title="Add to skills"
                          onClick={() => onAddSkill(s)}
                        >
                          +
                        </button>
                      </span>
                    ))}
                  </div>
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  )
}

// ─── Step 1: Personal Info ───────────────────────────────────
function StepPersonal({ data, onChange, errors }) {
  const set = (k) => (e) => onChange({ ...data, [k]: e.target.value })
  return (
    <div>
      <div className="section-heading">
        <h2>Personal information</h2>
        <p>Your name is required. All other fields are optional but recommended.</p>
      </div>
      <div className="form-grid">
        <Field label="Full Name" error={errors.name} className="full">
          <Input
            value={data.name}
            onChange={set('name')}
            placeholder="e.g. Ananya Mehta"
            className={errors.name ? 'error' : ''}
          />
        </Field>
        <Field label="Email" optional error={errors.email}>
          <Input
            type="email"
            value={data.email}
            onChange={set('email')}
            placeholder="you@example.com"
            className={errors.email ? 'error' : ''}
          />
        </Field>
        <Field label="Phone" optional error={errors.phone}
          hint="Include country code, e.g. +91 98765 43210">
          <Input
            value={data.phone}
            onChange={set('phone')}
            placeholder="+91 98765 43210"
            className={errors.phone ? 'error' : ''}
          />
        </Field>
        <Field label="GitHub Username" optional error={errors.github}
          hint="Just the username — not the full URL">
          <Input
            value={data.github}
            onChange={set('github')}
            placeholder="octocat"
            className={errors.github ? 'error' : ''}
          />
        </Field>
        <Field label="LinkedIn Username" optional error={errors.linkedin}
          hint="Just the username after linkedin.com/in/">
          <Input
            value={data.linkedin}
            onChange={set('linkedin')}
            placeholder="john-doe"
            className={errors.linkedin ? 'error' : ''}
          />
        </Field>
      </div>
    </div>
  )
}

// ─── Step 2: Education ───────────────────────────────────────
function StepEducation({ entries, onChange, cgpaScale, onCgpaScaleChange, errors }) {
  const addEntry = () => onChange([...entries, { college: '', cgpa: '' }])
  const removeEntry = (i) => onChange(entries.filter((_, j) => j !== i))
  const update = (i, field, val) =>
    onChange(entries.map((e, j) => j === i ? { ...e, [field]: val } : e))

  return (
    <div>
      <div className="section-heading">
        <h2>Education</h2>
        <p>Add your academic qualifications. CGPA scale depends on your institution.</p>
      </div>

      <div style={{ marginBottom: '1.5rem', display: 'flex', alignItems: 'center', gap: '1rem', flexWrap: 'wrap' }}>
        <span style={{ fontSize: '0.85rem', fontWeight: 600, color: 'var(--text-muted)' }}>CGPA Scale:</span>
        <div className="toggle-group">
          <button
            type="button"
            className={`toggle-btn ${cgpaScale === 10 ? 'active' : ''}`}
            onClick={() => onCgpaScaleChange(10)}
          >
            10-point scale
          </button>
          <button
            type="button"
            className={`toggle-btn ${cgpaScale === 4 ? 'active' : ''}`}
            onClick={() => onCgpaScaleChange(4)}
          >
            4-point scale
          </button>
        </div>
      </div>

      {entries.map((entry, i) => (
        <div className="card" key={i} style={{ marginBottom: '1rem' }}>
          <div className="card-header">
            <span className="card-title">Qualification #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeEntry(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div className="form-grid">
            <Field label="College / University" error={errors[`edu_${i}_college`]}>
              <Input
                value={entry.college}
                onChange={e => update(i, 'college', e.target.value)}
                placeholder="e.g. Vellore Institute of Technology"
              />
            </Field>
            <Field label={`CGPA (out of ${cgpaScale})`} error={errors[`edu_${i}_cgpa`]}>
              <Input
                value={entry.cgpa}
                onChange={e => update(i, 'cgpa', e.target.value)}
                placeholder={cgpaScale === 10 ? '9.30' : '3.80'}
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-olive btn-sm" onClick={addEntry}>
          <Icon name="plus" size={14}/> Add qualification
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 3: Skills ──────────────────────────────────────────
function StepSkills({ skills, onChange, jobAnalysis, onAddSkill }) {
  return (
    <div>
      <div className="section-heading">
        <h2>Technical Skills</h2>
        <p>List your programming languages, tools, frameworks, and databases.</p>
      </div>

      {jobAnalysis && jobAnalysis.missingSkills && jobAnalysis.missingSkills.length > 0 && (
        <div style={{
          background: 'var(--accent-light)',
          border: '1px solid rgba(184,92,56,0.3)',
          borderRadius: 'var(--radius-sm)',
          padding: '0.85rem 1rem',
          marginBottom: '1.25rem'
        }}>
          <div style={{ fontSize: '0.82rem', fontWeight: 600, color: 'var(--accent-dark)', marginBottom: 6 }}>
            💡 Target Job Recommendations (Click + to add):
          </div>
          <div className="skills-pills-wrap">
            {jobAnalysis.missingSkills.map((s, i) => (
              <span key={i} className="skill-pill missing">
                {s}
                <button
                  type="button"
                  className="skill-add-btn"
                  onClick={() => onAddSkill(s)}
                >
                  +
                </button>
              </span>
            ))}
          </div>
        </div>
      )}

      <Field label="Skills" hint="Type a skill and press Enter or comma. E.g. Python, C++, Docker, AWS, React">
        <SkillsInput skills={skills} onChange={onChange}/>
      </Field>
    </div>
  )
}

// ─── Step 4: Projects ────────────────────────────────────────
function StepProjects({ projects, onChange, errors }) {
  const addProject = () => onChange([...projects, { title: '', bullets: [''] }])
  const removeProject = (i) => onChange(projects.filter((_, j) => j !== i))
  const update = (i, field, val) =>
    onChange(projects.map((p, j) => j === i ? { ...p, [field]: val } : p))

  return (
    <div>
      <div className="section-heading">
        <h2>Projects</h2>
        <p>Showcase technical projects. Each project should have clear bullet points highlighting your impact.</p>
      </div>

      {projects.map((proj, i) => (
        <div className="card" key={i} style={{ marginBottom: '1rem' }}>
          <div className="card-header">
            <span className="card-title">Project #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeProject(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            <Field label="Project Title" error={errors[`proj_${i}_title`]}>
              <Input
                value={proj.title || proj.name || ''}
                onChange={e => update(i, 'title', e.target.value)}
                placeholder="e.g. Distributed Task Queue"
              />
            </Field>
            <Field label="Key Highlights & Bullets">
              <BulletList
                bullets={proj.bullets || []}
                onChange={b => update(i, 'bullets', b)}
                placeholder="e.g. Engineered microservices in C++ with 99.9% uptime"
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-olive btn-sm" onClick={addProject}>
          <Icon name="plus" size={14}/> Add project
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 5: Experience ──────────────────────────────────────
function StepExperience({ experience, onChange, errors }) {
  const addExp = () =>
    onChange([...experience, { company: '', role: '', duration: '', bullets: [''] }])
  const removeExp = (i) => onChange(experience.filter((_, j) => j !== i))
  const update = (i, field, val) =>
    onChange(experience.map((e, j) => j === i ? { ...e, [field]: val } : e))

  return (
    <div>
      <div className="section-heading">
        <h2>Experience</h2>
        <p>Add internships, full-time positions, or research roles.</p>
      </div>

      {experience.map((exp, i) => (
        <div className="card" key={i} style={{ marginBottom: '1rem' }}>
          <div className="card-header">
            <span className="card-title">Role #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeExp(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            <div className="form-grid triple">
              <Field label="Company / Organization" error={errors[`exp_${i}_company`]}>
                <Input
                  value={exp.company}
                  onChange={e => update(i, 'company', e.target.value)}
                  placeholder="e.g. Google"
                />
              </Field>
              <Field label="Role / Title" optional>
                <Input
                  value={exp.role || ''}
                  onChange={e => update(i, 'role', e.target.value)}
                  placeholder="Software Engineer Intern"
                />
              </Field>
              <Field label="Duration" optional hint="e.g. May 2024 - Aug 2024">
                <Input
                  value={exp.duration || ''}
                  onChange={e => update(i, 'duration', e.target.value)}
                  placeholder="May 2024 - Aug 2024"
                />
              </Field>
            </div>
            <Field label="Accomplishments & Impact">
              <BulletList
                bullets={exp.bullets || []}
                onChange={b => update(i, 'bullets', b)}
                placeholder="e.g. Reduced latency by 35% by rewriting caching layer"
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-olive btn-sm" onClick={addExp}>
          <Icon name="plus" size={14}/> Add experience
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 6: Extras ──────────────────────────────────────────
function StepExtras({ extras, onChange }) {
  const addSection = () => onChange([...extras, { title: '', items: [{ text: '', subbullets: [] }] }])
  const removeSection = (i) => onChange(extras.filter((_, j) => j !== i))
  const updateSection = (i, field, val) =>
    onChange(extras.map((s, j) => j === i ? { ...s, [field]: val } : s))

  const addItem = (si) => {
    const section = { ...extras[si], items: [...extras[si].items, { text: '', subbullets: [] }] }
    onChange(extras.map((s, j) => j === si ? section : s))
  }
  const removeItem = (si, ii) => {
    const section = { ...extras[si], items: extras[si].items.filter((_, j) => j !== ii) }
    onChange(extras.map((s, j) => j === si ? section : s))
  }
  const updateItem = (si, ii, field, val) => {
    const section = {
      ...extras[si],
      items: extras[si].items.map((it, j) => j === ii ? { ...it, [field]: val } : it)
    }
    onChange(extras.map((s, j) => j === si ? section : s))
  }

  return (
    <div>
      <div className="section-heading">
        <h2>Custom Sections</h2>
        <p>Add any additional sections: Publications, Certifications, Awards, etc.</p>
      </div>

      {extras.map((section, si) => (
        <div className="card" key={si} style={{ marginBottom: '1rem' }}>
          <div className="card-header">
            <span className="card-title">Section #{si + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeSection(si)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            <Field label="Section Title">
              <Input
                value={section.title}
                onChange={e => updateSection(si, 'title', e.target.value)}
                placeholder="e.g. Publications, Awards, Certifications"
              />
            </Field>
            {section.items.map((item, ii) => (
              <div key={ii} style={{
                background: 'var(--bg)',
                border: '1px solid var(--border)',
                borderRadius: 'var(--radius-sm)',
                padding: '0.85rem',
                display: 'flex',
                flexDirection: 'column',
                gap: '0.75rem'
              }}>
                <div style={{ display: 'flex', alignItems: 'flex-start', gap: '0.5rem' }}>
                  <div style={{ flex: 1 }}>
                    <Field label="Entry Text">
                      <Input
                        value={item.text}
                        onChange={e => updateItem(si, ii, 'text', e.target.value)}
                        placeholder="e.g. Paper title or award name"
                      />
                    </Field>
                  </div>
                  <button className="btn btn-icon btn-danger" style={{ marginTop: '1.4rem' }}
                    onClick={() => removeItem(si, ii)}>
                    <Icon name="trash" size={14}/>
                  </button>
                </div>
                <Field label="Sub-bullets" optional>
                  <BulletList
                    bullets={item.subbullets}
                    onChange={v => updateItem(si, ii, 'subbullets', v)}
                    placeholder="Additional detail…"
                  />
                </Field>
              </div>
            ))}
            <button className="btn btn-ghost btn-sm" onClick={() => addItem(si)}
              style={{ alignSelf: 'flex-start' }}>
              <Icon name="plus" size={13}/> Add entry
            </button>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-olive btn-sm" onClick={addSection}>
          <Icon name="plus" size={14}/> Add custom section
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Upload & Incremental Tailoring View ──────────────────────
function UploadTailorView({ onResumeImported, jobUrl, setJobUrl, onAnalyze, jobAnalysis, onAddSkill, analyzing }) {
  const [uploading, setUploading] = useState(false)
  const [error, setError] = useState(null)
  const [dragOver, setDragOver] = useState(false)
  const fileInputRef = useRef()

  const handleFile = async (file) => {
    if (!file) return
    setError(null)
    setUploading(true)
    const fd = new FormData()
    fd.append('resumeFile', file)
    if (jobUrl) fd.append('jobUrl', jobUrl)

    try {
      const res = await fetch('/api/upload-resume', { method: 'POST', body: fd })
      const data = await res.json()
      if (!res.ok || !data.success) {
        throw new Error(data.error || 'Failed to extract resume from PDF')
      }
      onResumeImported(data.resume, data.jobAnalysis, data.dslContent)
    } catch (err) {
      setError(err.message)
    } finally {
      setUploading(false)
    }
  }

  return (
    <div>
      <div className="section-heading">
        <h2>Upload Existing Resume & Incremental Job Tailoring</h2>
        <p>
          Upload your existing resume PDF. We extract all your candidate details, map them against target job requirements,
          and highlight only what needs to be added or changed — keeping the rest 100% intact.
        </p>
      </div>

      <JobMatcherPanel
        jobUrl={jobUrl}
        setJobUrl={setJobUrl}
        jobAnalysis={jobAnalysis}
        onAnalyze={onAnalyze}
        onAddSkill={onAddSkill}
        analyzing={analyzing}
      />

      {error && (
        <div className="alert alert-error" style={{ marginBottom: '1.5rem' }}>
          <span className="alert-icon"><Icon name="alert" size={16}/></span>
          <div>{error}</div>
        </div>
      )}

      <div
        className={`upload-dropzone ${dragOver ? 'dragover' : ''}`}
        onDragOver={e => { e.preventDefault(); setDragOver(true) }}
        onDragLeave={() => setDragOver(false)}
        onDrop={e => {
          e.preventDefault()
          setDragOver(false)
          if (e.dataTransfer.files?.[0]) handleFile(e.dataTransfer.files[0])
        }}
        onClick={() => fileInputRef.current?.click()}
      >
        <input
          ref={fileInputRef}
          type="file"
          accept=".pdf,.resume"
          style={{ display: 'none' }}
          onChange={e => {
            if (e.target.files?.[0]) handleFile(e.target.files[0])
          }}
        />
        <div style={{ color: 'var(--accent)', marginBottom: 4 }}>
          <Icon name="upload" size={36}/>
        </div>
        <h3>{uploading ? 'Extracting Resume Data with C11 Parser…' : 'Drop your resume PDF here or click to browse'}</h3>
        <p>
          Supports <strong>.pdf</strong> resumes or <strong>.resume</strong> DSL files.
          Our C11 parser extracts Name, Contact info, Education, Skills, Projects, and Experience automatically.
        </p>
      </div>
    </div>
  )
}

// ─── Live DSL Playground ─────────────────────────────────────
function DslPlayground({ dsl, setDsl, onCompile, loading, error, diagnostics, jobUrl, setJobUrl, onAnalyze, jobAnalysis, onAddSkill, analyzing }) {
  const loadSample = () => {
    setDsl(
`name "Ananya Mehta"
phone "+91 98765 43210"
email "ananya.mehta@example.com"
github "ananya-mehta"
linkedin "ananya-mehta"

education
college "VIT"
cgpa 9.30

skills
C++
Python
Java
Docker
Kubernetes

projects
project "Resume Compiler"
bullet "DSL-to-PDF resume generator built with flex and bison"
bullet "Implements full compiler pipeline: lex, parse, AST, semantic checks, codegen"

experience
company "Google"
role "Software Engineering Intern"
duration "May 2024 - Aug 2024"
bullet "Built internal tooling that reduced CI pipeline latency by 30%"
bullet "Collaborated with a team of 5 to design a new API surface"
`)
  }

  return (
    <div className="code-editor-box">
      <div className="section-heading">
        <h2>Compiler DSL Live Playground</h2>
        <p>Write raw <code>.resume</code> code directly to invoke lexer, parser, semantic analyzer, and pdflatex in real time.</p>
      </div>

      <JobMatcherPanel
        jobUrl={jobUrl}
        setJobUrl={setJobUrl}
        jobAnalysis={jobAnalysis}
        onAnalyze={onAnalyze}
        onAddSkill={onAddSkill}
        analyzing={analyzing}
      />

      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <button className="btn btn-ghost btn-sm" onClick={loadSample}>
          Load Sample Resume DSL
        </button>
        <button
          className="btn btn-primary btn-sm"
          onClick={() => onCompile(dsl)}
          disabled={loading || !dsl.trim()}
        >
          <Icon name="terminal" size={14}/> {loading ? 'Compiling…' : 'Compile .resume ➔ PDF'}
        </button>
      </div>

      {error && (
        <div className="alert alert-error">
          <span className="alert-icon"><Icon name="alert" size={16}/></span>
          <div>
            <strong>Compiler Error</strong>
            <div style={{ marginTop: 4, fontSize: '0.82rem', whiteSpace: 'pre-wrap' }}>{error}</div>
            {diagnostics && diagnostics.length > 0 && (
              <div style={{ marginTop: 6 }}>
                {diagnostics.map((d, i) => (
                  <div key={i} style={{ fontFamily: 'var(--font-mono)', fontSize: '0.78rem' }}>
                    Line {d.line}, Column {d.col}: {d.message}
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      )}

      <textarea
        className="code-textarea"
        value={dsl}
        onChange={e => setDsl(e.target.value)}
        placeholder="Type .resume DSL code here..."
        spellCheck="false"
      />
    </div>
  )
}

// ─── AST Visualizer Component ────────────────────────────────
function AstViewer({ ast }) {
  if (!ast) return <div style={{ color: 'var(--text-muted)' }}>No AST available.</div>

  const renderVal = (v) => {
    if (v === null || v === undefined) return <span style={{ color: '#888' }}>null</span>
    if (typeof v === 'boolean') return <span style={{ color: '#d2876f' }}>{v ? 'true' : 'false'}</span>
    if (typeof v === 'number') return <span style={{ color: '#b5cea8' }}>{v}</span>
    if (typeof v === 'string') return <span style={{ color: '#ce9178' }}>"{v}"</span>
    if (Array.isArray(v)) {
      if (v.length === 0) return <span>[]</span>
      return (
        <div>
          <span>[</span>
          <div style={{ paddingLeft: '1rem' }}>
            {v.map((item, idx) => (
              <div key={idx}>
                {renderVal(item)}
                {idx < v.length - 1 && ','}
              </div>
            ))}
          </div>
          <span>]</span>
        </div>
      )
    }
    if (typeof v === 'object') {
      const keys = Object.keys(v)
      return (
        <div>
          <span>{'{'}</span>
          <div style={{ paddingLeft: '1rem' }}>
            {keys.map((k, idx) => (
              <div key={k}>
                <span className="ast-key">"{k}"</span>: {renderVal(v[k])}
                {idx < keys.length - 1 && ','}
              </div>
            ))}
          </div>
          <span>{'}'}</span>
        </div>
      )
    }
    return String(v)
  }

  return (
    <div className="ast-container">
      {renderVal(ast)}
    </div>
  )
}

// ─── Result Screen ───────────────────────────────────────────
function ResultScreen({ result, onReset }) {
  const [activeTab, setActiveTab] = useState('pdf')
  const [copied, setCopied] = useState(false)
  const pdfUrl = result.pdfUrl

  const handleDownload = () => {
    const a = document.createElement('a')
    a.href = pdfUrl
    a.download = result.pdfFilename
    a.click()
  }

  const handleCopy = (text) => {
    navigator.clipboard.writeText(text)
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  const ja = result.jobAnalysis

  return (
    <div className="result-screen">
      <div className="alert alert-success">
        <span className="alert-icon">✓</span>
        <div style={{ flex: 1 }}>
          <strong>Resume compiled and generated successfully!</strong>
          <div style={{ marginTop: 4, fontSize: '0.82rem', opacity: 0.85 }}>
            C11 compiler finished: lex ➔ bison parse ➔ AST ➔ semantic checks ➔ codegen ➔ pdflatex
          </div>
        </div>
        {ja && (
          <span className={`match-badge ${ja.matchPercentage >= 75 ? 'high' : ja.matchPercentage >= 45 ? 'med' : 'low'}`}>
            ★ {ja.matchPercentage}% Job Match
          </span>
        )}
      </div>

      <div className="result-header">
        <h2 style={{ fontFamily: 'var(--font-serif)' }}>{result.pdfFilename}</h2>
        <div className="result-actions">
          <a className="btn btn-secondary btn-sm" href={pdfUrl} target="_blank" rel="noopener noreferrer">
            <Icon name="ext" size={14}/> Open in tab
          </a>
          <button className="btn btn-primary btn-sm" onClick={handleDownload}>
            <Icon name="dl" size={14}/> Download PDF
          </button>
          <button className="btn btn-ghost btn-sm" onClick={onReset}>
            <Icon name="refresh" size={14}/> Build Another
          </button>
        </div>
      </div>

      {/* Result Tabs */}
      <div className="tab-bar">
        <button
          className={`tab-btn ${activeTab === 'pdf' ? 'active' : ''}`}
          onClick={() => setActiveTab('pdf')}
        >
          <Icon name="doc" size={14}/> PDF Preview
        </button>
        <button
          className={`tab-btn ${activeTab === 'dsl' ? 'active' : ''}`}
          onClick={() => setActiveTab('dsl')}
        >
          <Icon name="code" size={14}/> Generated .resume DSL
        </button>
        {result.texContent && (
          <button
            className={`tab-btn ${activeTab === 'tex' ? 'active' : ''}`}
            onClick={() => setActiveTab('tex')}
          >
            <Icon name="terminal" size={14}/> LaTeX (.tex)
          </button>
        )}
        {ja && (
          <button
            className={`tab-btn ${activeTab === 'job' ? 'active' : ''}`}
            onClick={() => setActiveTab('job')}
          >
            <Icon name="target" size={14}/> ATS Job Match Report
          </button>
        )}
        <button
          className={`tab-btn ${activeTab === 'ast' ? 'active' : ''}`}
          onClick={() => setActiveTab('ast')}
        >
          <Icon name="tree" size={14}/> Compiler AST
        </button>
      </div>

      {/* Tab Panels */}
      {activeTab === 'pdf' && (
        <div className="pdf-preview-container">
          <iframe
            src={`${pdfUrl}#toolbar=1&view=FitH`}
            title="Resume PDF Preview"
          />
        </div>
      )}

      {activeTab === 'dsl' && (
        <div>
          <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: 8 }}>
            <button className="btn btn-secondary btn-sm" onClick={() => handleCopy(result.dslContent)}>
              <Icon name="copy" size={14}/> {copied ? 'Copied!' : 'Copy DSL'}
            </button>
          </div>
          <pre className="dsl-preview">{result.dslContent}</pre>
        </div>
      )}

      {activeTab === 'tex' && (
        <div>
          <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: 8 }}>
            <button className="btn btn-secondary btn-sm" onClick={() => handleCopy(result.texContent)}>
              <Icon name="copy" size={14}/> {copied ? 'Copied!' : 'Copy LaTeX'}
            </button>
          </div>
          <pre className="dsl-preview">{result.texContent}</pre>
        </div>
      )}

      {activeTab === 'job' && ja && (
        <div className="card">
          <div className="card-header">
            <span className="card-title">Job Requirements Compatibility Analysis</span>
            <span className={`match-badge ${ja.matchPercentage >= 75 ? 'high' : 'med'}`}>
              {ja.matchPercentage}% Match Score
            </span>
          </div>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)', marginBottom: '1rem' }}>
            Target: <code>{ja.source}</code>
          </p>
          <div style={{ marginBottom: '1rem' }}>
            <h4 style={{ fontSize: '0.85rem', color: 'var(--success)', marginBottom: 6 }}>
              ✓ Matched Skills ({ja.matchedCount}):
            </h4>
            <div className="skills-pills-wrap">
              {ja.matchedSkills?.map((s, i) => (
                <span key={i} className="skill-pill matched"><Icon name="check" size={11}/> {s}</span>
              ))}
            </div>
          </div>
          {ja.missingCount > 0 && (
            <div>
              <h4 style={{ fontSize: '0.85rem', color: 'var(--accent-dark)', marginBottom: 6 }}>
                ⚠️ Missing Target Skills ({ja.missingCount}):
              </h4>
              <div className="skills-pills-wrap">
                {ja.missingSkills?.map((s, i) => (
                  <span key={i} className="skill-pill missing">{s}</span>
                ))}
              </div>
            </div>
          )}
        </div>
      )}

      {activeTab === 'ast' && (
        <div>
          <div style={{ fontSize: '0.8rem', color: 'var(--text-muted)', marginBottom: 8 }}>
            Visual representation of AST nodes generated by Bison parser:
          </div>
          <AstViewer ast={result.ast || result.resume || { name: result.pdfFilename }} />
        </div>
      )}
    </div>
  )
}

// ─── Main App ────────────────────────────────────────────────
const STEPS = [
  { id: 'personal',   label: 'Personal' },
  { id: 'education',  label: 'Education' },
  { id: 'skills',     label: 'Skills' },
  { id: 'projects',   label: 'Projects' },
  { id: 'experience', label: 'Experience' },
  { id: 'extras',     label: 'Custom' },
  { id: 'result',     label: 'Result' },
]

const INITIAL = {
  personal: { name: '', email: '', phone: '', github: '', linkedin: '' },
  education: [],
  skills: [],
  projects: [],
  experience: [],
  extras: [],
  cgpaScale: 10,
}

export default function App() {
  const [appMode, setAppMode] = useState('wizard') // 'wizard' | 'upload' | 'dsl'
  const [step, setStep] = useState(0)
  const [formData, setFormData] = useState(INITIAL)
  const [errors, setErrors] = useState({})
  const [loading, setLoading] = useState(false)
  const [result, setResult] = useState(null)
  const [apiError, setApiError] = useState(null)
  const [diagnostics, setDiagnostics] = useState([])

  // Target Job Matcher State
  const [jobUrl, setJobUrl] = useState('')
  const [jobDesc, setJobDesc] = useState('')
  const [jobAnalysis, setJobAnalysis] = useState(null)
  const [analyzingJob, setAnalyzingJob] = useState(false)

  // Live DSL State
  const [dslDraft, setDslDraft] = useState('')

  // Compiler Status
  const [compilerHealth, setCompilerHealth] = useState({ online: false, platform: 'loading' })

  // Check backend health on mount
  useEffect(() => {
    fetch('/api/health')
      .then(res => res.json())
      .then(data => setCompilerHealth({ online: data.status === 'ok', platform: data.platform }))
      .catch(() => setCompilerHealth({ online: false, platform: 'offline' }))
  }, [])

  // Analyze Job URL
  const handleAnalyzeJob = async () => {
    if (!jobUrl.trim()) return
    setAnalyzingJob(true)
    try {
      const res = await fetch('/api/analyze-job', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          jobUrl: jobUrl.trim(),
          currentSkills: formData.skills,
        }),
      })
      const data = await res.json()
      if (data.success && data.jobAnalysis) {
        setJobAnalysis(data.jobAnalysis)
      }
    } catch (_) {
      // silently fallback
    } finally {
      setAnalyzingJob(false)
    }
  }

  // Quick add skill from missing job requirements
  const handleAddSkill = (skill) => {
    if (!skill || !skill.trim()) return
    const val = skill.trim()
    if (!formData.skills.find(s => s.toLowerCase() === val.toLowerCase())) {
      const updatedSkills = [...formData.skills, val]
      setFormData(prev => ({ ...prev, skills: updatedSkills }))

      // Update match analysis locally
      if (jobAnalysis) {
        const remainingMissing = (jobAnalysis.missingSkills || []).filter(
          s => s.toLowerCase() !== val.toLowerCase()
        )
        const updatedMatched = [...(jobAnalysis.matchedSkills || []), val]
        const total = (jobAnalysis.jobSkills || []).length
        const newScore = total > 0 ? (updatedMatched.length / total) * 100 : 100

        setJobAnalysis({
          ...jobAnalysis,
          matchedSkills: updatedMatched,
          matchedCount: updatedMatched.length,
          missingSkills: remainingMissing,
          missingCount: remainingMissing.length,
          matchPercentage: Math.round(newScore * 10) / 10,
        })
      }
    }
  }

  // Handle uploaded resume from PDF
  const handleResumeImported = (importedResume, importedJobAnalysis, dsl) => {
    setFormData({
      personal: {
        name: importedResume.name || '',
        email: importedResume.email || '',
        phone: importedResume.phone || '',
        github: importedResume.github || '',
        linkedin: importedResume.linkedin || '',
      },
      education: (importedResume.education || []).map(e => ({
        college: e.college || '',
        cgpa: String(e.cgpa || 9.0),
      })),
      skills: importedResume.skills || [],
      projects: importedResume.projects || [],
      experience: importedResume.experience || [],
      extras: importedResume.extras || [],
      cgpaScale: 10,
    })

    if (importedJobAnalysis) {
      setJobAnalysis(importedJobAnalysis)
    }
    if (dsl) {
      setDslDraft(dsl)
    }

    setAppMode('wizard')
    setStep(0)
  }

  // Compile Raw DSL from Playground
  const handleCompileDsl = async (code) => {
    setLoading(true)
    setApiError(null)
    setDiagnostics([])
    try {
      const res = await fetch('/api/compile-dsl', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          dslContent: code,
          jobUrl: jobUrl.trim() || undefined,
        }),
      })
      const data = await res.json()
      if (!res.ok || !data.success) {
        setDiagnostics(data.diagnostics || [])
        throw new Error(data.error || 'Compilation failed.')
      }
      setResult(data)
      setStep(6)
    } catch (err) {
      setApiError(err.message)
    } finally {
      setLoading(false)
    }
  }

  const visibleSteps = STEPS.slice(0, 6)

  const validate = () => {
    let errs = {}
    if (step === 0) errs = validatePersonal(formData.personal)
    if (step === 1) errs = validateEducation(formData.education)
    if (step === 3) errs = validateProjects(formData.projects)
    if (step === 4) errs = validateExperience(formData.experience)
    setErrors(errs)
    return Object.keys(errs).length === 0
  }

  const next = () => {
    if (!validate()) return
    if (step === 5) {
      generate()
    } else {
      setStep(s => s + 1)
    }
  }

  const back = () => {
    setErrors({})
    setStep(s => s - 1)
  }

  const generate = async () => {
    setApiError(null)
    setLoading(true)
    setDiagnostics([])
    try {
      const payload = {
        name: formData.personal.name.trim(),
        email: formData.personal.email.trim() || undefined,
        phone: formData.personal.phone.trim() || undefined,
        github: formData.personal.github.trim() || undefined,
        linkedin: formData.personal.linkedin.trim() || undefined,
        education: formData.education,
        skills: formData.skills,
        projects: formData.projects,
        experience: formData.experience,
        extras: formData.extras,
        cgpaScale: formData.cgpaScale,
        jobUrl: jobUrl.trim() || undefined,
      }
      const res = await fetch('/api/generate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      })
      const data = await res.json()
      if (!res.ok || !data.success) {
        setDiagnostics(data.diagnostics || [])
        throw new Error(data.error || 'Generation failed.')
      }
      setResult(data)
      setStep(6)
    } catch (err) {
      setApiError(err.message)
    } finally {
      setLoading(false)
    }
  }

  const reset = () => {
    setFormData(INITIAL)
    setErrors({})
    setResult(null)
    setApiError(null)
    setDiagnostics([])
    setStep(0)
  }

  return (
    <div className="app-shell">
      {/* Header */}
      <header className="app-header">
        <div style={{ display: 'flex', alignItems: 'center', gap: '1.25rem' }}>
          <div className="app-header__wordmark">
            <Icon name="doc" size={18}/>
            resume<span>c</span>
            <span className="app-header__badge">C11 compiler</span>
          </div>

          {/* Mode Switcher */}
          <div className="nav-modes">
            <button
              className={`nav-mode-btn ${appMode === 'wizard' ? 'active' : ''}`}
              onClick={() => { setAppMode('wizard'); if (step === 6) setStep(0) }}
            >
              <Icon name="doc" size={13}/> Interactive Builder
            </button>
            <button
              className={`nav-mode-btn ${appMode === 'upload' ? 'active' : ''}`}
              onClick={() => setAppMode('upload')}
            >
              <Icon name="upload" size={13}/> Upload & Tailor PDF
            </button>
            <button
              className={`nav-mode-btn ${appMode === 'dsl' ? 'active' : ''}`}
              onClick={() => setAppMode('dsl')}
            >
              <Icon name="terminal" size={13}/> DSL Playground
            </button>
          </div>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
          <span style={{
            fontSize: '0.75rem',
            display: 'inline-flex',
            alignItems: 'center',
            gap: 5,
            color: compilerHealth.online ? 'var(--success)' : 'var(--error)'
          }}>
            <span style={{
              width: 7,
              height: 7,
              borderRadius: '50%',
              backgroundColor: compilerHealth.online ? 'var(--success)' : 'var(--error)'
            }}/>
            {compilerHealth.online ? 'Compiler Online' : 'Compiler Offline'}
          </span>

          {appMode === 'wizard' && step < 6 && (
            <span style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
              Step {step + 1} of 6
            </span>
          )}
        </div>
      </header>

      {loading && (
        <div className="loading-overlay">
          <div className="spinner"/>
          <div className="loading-text">Compiling via resumec pipeline…</div>
          <p style={{ fontSize: '0.8rem', color: 'var(--text-faint)' }}>
            Running flex ➔ bison ➔ AST ➔ semantic checks ➔ codegen ➔ pdflatex
          </p>
        </div>
      )}

      <main className="main-content">
        {step === 6 && result ? (
          <ResultScreen result={result} onReset={reset} />
        ) : appMode === 'upload' ? (
          <UploadTailorView
            onResumeImported={handleResumeImported}
            jobUrl={jobUrl}
            setJobUrl={setJobUrl}
            onAnalyze={handleAnalyzeJob}
            jobAnalysis={jobAnalysis}
            onAddSkill={handleAddSkill}
            analyzing={analyzingJob}
          />
        ) : appMode === 'dsl' ? (
          <DslPlayground
            dsl={dslDraft}
            setDsl={setDslDraft}
            onCompile={handleCompileDsl}
            loading={loading}
            error={apiError}
            diagnostics={diagnostics}
            jobUrl={jobUrl}
            setJobUrl={setJobUrl}
            onAnalyze={handleAnalyzeJob}
            jobAnalysis={jobAnalysis}
            onAddSkill={handleAddSkill}
            analyzing={analyzingJob}
          />
        ) : (
          <>
            {/* Target Job Scraper & Requirements Bar */}
            <JobMatcherPanel
              jobUrl={jobUrl}
              setJobUrl={setJobUrl}
              jobAnalysis={jobAnalysis}
              onAnalyze={handleAnalyzeJob}
              onAddSkill={handleAddSkill}
              analyzing={analyzingJob}
            />

            {/* Step progress bar */}
            <div className="step-bar">
              {visibleSteps.map((s, i) => (
                <div
                  key={s.id}
                  className={`step-item ${i < step ? 'done' : ''} ${i === step ? 'active' : ''}`}
                >
                  <div className="step-num">
                    {i < step ? <Icon name="check" size={12}/> : i + 1}
                  </div>
                  <span className="step-label">{s.label}</span>
                </div>
              ))}
            </div>

            {/* Error banner */}
            {apiError && (
              <div className="alert alert-error" style={{ marginBottom: '1.5rem' }}>
                <span className="alert-icon"><Icon name="alert" size={16}/></span>
                <div>
                  <strong>Compilation failed</strong>
                  <div style={{ marginTop: 4, fontSize: '0.82rem', whiteSpace: 'pre-wrap', opacity: 0.9 }}>
                    {apiError}
                  </div>
                  {diagnostics && diagnostics.length > 0 && (
                    <div style={{ marginTop: 6 }}>
                      {diagnostics.map((d, i) => (
                        <div key={i} style={{ fontFamily: 'var(--font-mono)', fontSize: '0.78rem' }}>
                          Error on Line {d.line}, Column {d.col}: {d.message}
                        </div>
                      ))}
                    </div>
                  )}
                </div>
              </div>
            )}

            {/* Step content */}
            {step === 0 && (
              <StepPersonal
                data={formData.personal}
                onChange={v => setFormData(d => ({ ...d, personal: v }))}
                errors={errors}
              />
            )}
            {step === 1 && (
              <StepEducation
                entries={formData.education}
                onChange={v => setFormData(d => ({ ...d, education: v }))}
                cgpaScale={formData.cgpaScale}
                onCgpaScaleChange={v => setFormData(d => ({ ...d, cgpaScale: v }))}
                errors={errors}
              />
            )}
            {step === 2 && (
              <StepSkills
                skills={formData.skills}
                onChange={v => setFormData(d => ({ ...d, skills: v }))}
                jobAnalysis={jobAnalysis}
                onAddSkill={handleAddSkill}
              />
            )}
            {step === 3 && (
              <StepProjects
                projects={formData.projects}
                onChange={v => setFormData(d => ({ ...d, projects: v }))}
                errors={errors}
              />
            )}
            {step === 4 && (
              <StepExperience
                experience={formData.experience}
                onChange={v => setFormData(d => ({ ...d, experience: v }))}
                errors={errors}
              />
            )}
            {step === 5 && (
              <StepExtras
                extras={formData.extras}
                onChange={v => setFormData(d => ({ ...d, extras: v }))}
              />
            )}

            {/* Navigation */}
            <div className="step-nav">
              {step > 0 ? (
                <button className="btn btn-secondary" onClick={back}>
                  <Icon name="arrow_l" size={15}/> Back
                </button>
              ) : (
                <div/>
              )}
              <button
                className="btn btn-primary btn-lg"
                onClick={next}
                disabled={loading}
              >
                {step === 5 ? (
                  <><Icon name="doc" size={16}/> Compile & Generate Resume</>
                ) : (
                  <>Continue <Icon name="arrow_r" size={15}/></>
                )}
              </button>
            </div>
          </>
        )}
      </main>
    </div>
  )
}
