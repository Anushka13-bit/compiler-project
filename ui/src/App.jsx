import { useState, useRef, useCallback } from 'react'

// ─── Icons ──────────────────────────────────────────────────
const Icon = ({ name, size = 16 }) => {
  const icons = {
    plus:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="8" y1="2" x2="8" y2="14"/><line x1="2" y1="8" x2="14" y2="8"/></svg>,
    trash:   <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="2,4 14,4"/><path d="M5,4V2h6v2"/><rect x="3" y="4" width="10" height="10" rx="1"/><line x1="6" y1="7" x2="6" y2="11"/><line x1="10" y1="7" x2="10" y2="11"/></svg>,
    check:   <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="2,8 6,12 14,4"/></svg>,
    arrow_r: <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="2" y1="8" x2="14" y2="8"/><polyline points="9,3 14,8 9,13"/></svg>,
    arrow_l: <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="14" y1="8" x2="2" y2="8"/><polyline points="7,3 2,8 7,13"/></svg>,
    down:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="3,5 8,11 13,5"/></svg>,
    dl:      <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M8 2v8M5 7l3 4 3-4"/><path d="M2 12v2h12v-2"/></svg>,
    ext:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M9 2h5v5"/><line x1="14" y1="2" x2="7" y2="9"/><path d="M7 4H3a1 1 0 00-1 1v8a1 1 0 001 1h8a1 1 0 001-1V9"/></svg>,
    code:    <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><polyline points="5,4 1,8 5,12"/><polyline points="11,4 15,8 11,12"/></svg>,
    alert:   <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M8 2L1 14h14L8 2z"/><line x1="8" y1="7" x2="8" y2="10"/><circle cx="8" cy="12.5" r="0.5" fill="currentColor"/></svg>,
    refresh: <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><path d="M13.5 2.5A6.5 6.5 0 012 9"/><polyline points="11,0 13.5,2.5 11,5"/><path d="M2.5 13.5A6.5 6.5 0 0014 7"/><polyline points="5,11 2.5,13.5 5,16"/></svg>,
    doc:     <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><rect x="2" y="1" width="12" height="14" rx="1.5"/><line x1="5" y1="5" x2="11" y2="5"/><line x1="5" y1="8" x2="11" y2="8"/><line x1="5" y1="11" x2="9" y2="11"/></svg>,
    x:       <svg width={size} height={size} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.8"><line x1="3" y1="3" x2="13" y2="13"/><line x1="13" y1="3" x2="3" y2="13"/></svg>,
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
    if (!e.title.trim()) errors[`proj_${i}_title`] = 'Project title is required.'
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
function Field({ label, optional, hint, error, children }) {
  return (
    <div className="field">
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
function BulletList({ bullets, onChange, placeholder = 'Add a bullet point…' }) {
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
          <button className={`toggle-btn ${cgpaScale === 10 ? 'active' : ''}`}
            onClick={() => onCgpaScaleChange(10)}>Out of 10</button>
          <button className={`toggle-btn ${cgpaScale === 4 ? 'active' : ''}`}
            onClick={() => onCgpaScaleChange(4)}>Out of 4</button>
        </div>
      </div>

      {entries.map((entry, i) => (
        <div className="card" key={i}>
          <div className="card-header">
            <span className="card-title">Education #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeEntry(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div className="form-grid">
            <Field label="College / University" error={errors[`edu_${i}_college`]}
              style={{ gridColumn: '1 / -1' }}>
              <Input
                value={entry.college}
                onChange={e => update(i, 'college', e.target.value)}
                placeholder="e.g. VIT, IIT Bombay"
                className={errors[`edu_${i}_college`] ? 'error' : ''}
              />
            </Field>
            <Field label={`CGPA (0 – ${cgpaScale})`} error={errors[`edu_${i}_cgpa`]}>
              <Input
                type="number"
                step="0.01"
                min="0"
                max={cgpaScale}
                value={entry.cgpa}
                onChange={e => update(i, 'cgpa', e.target.value)}
                placeholder={cgpaScale === 10 ? '9.30' : '3.8'}
                className={errors[`edu_${i}_cgpa`] ? 'error' : ''}
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-secondary btn-sm" onClick={addEntry}>
          <Icon name="plus" size={14}/> Add education
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 3: Skills ──────────────────────────────────────────
function StepSkills({ skills, onChange }) {
  return (
    <div>
      <div className="section-heading">
        <h2>Skills</h2>
        <p>Type a skill and press <kbd style={{ background: 'var(--bg-input)', padding: '1px 6px', borderRadius: 3, border: '1px solid var(--border)', fontSize: '0.8rem', fontFamily: 'var(--font-mono)' }}>Enter</kbd> or comma to add. Skills are case-insensitive duplicates will be ignored.</p>
      </div>
      <Field label="Your Skills">
        <SkillsInput skills={skills} onChange={onChange}/>
      </Field>
      {skills.length > 0 && (
        <p style={{ marginTop: '0.75rem', fontSize: '0.8rem', color: 'var(--text-muted)' }}>
          {skills.length} skill{skills.length > 1 ? 's' : ''} added
        </p>
      )}
    </div>
  )
}

// ─── Step 4: Projects ────────────────────────────────────────
function StepProjects({ projects, onChange, errors }) {
  const addProject = () => onChange([...projects, { title: '', bullets: [''] }])
  const removeProject = (i) => onChange(projects.filter((_, j) => j !== i))
  const updateProject = (i, field, val) =>
    onChange(projects.map((p, j) => j === i ? { ...p, [field]: val } : p))

  return (
    <div>
      <div className="section-heading">
        <h2>Projects</h2>
        <p>Showcase your work. Each project can have multiple bullet points.</p>
      </div>

      {projects.map((proj, i) => (
        <div className="card" key={i}>
          <div className="card-header">
            <span className="card-title">Project #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeProject(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            <Field label="Project Title" error={errors[`proj_${i}_title`]}>
              <Input
                value={proj.title}
                onChange={e => updateProject(i, 'title', e.target.value)}
                placeholder="e.g. Resume Compiler"
                className={errors[`proj_${i}_title`] ? 'error' : ''}
              />
            </Field>
            <Field label="Bullet Points" optional>
              <BulletList
                bullets={proj.bullets}
                onChange={v => updateProject(i, 'bullets', v)}
                placeholder="Describe what you built or achieved…"
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-secondary btn-sm" onClick={addProject}>
          <Icon name="plus" size={14}/> Add project
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 5: Experience ──────────────────────────────────────
function StepExperience({ experience, onChange, errors }) {
  const addExp = () => onChange([...experience, { company: '', role: '', duration: '', bullets: [] }])
  const removeExp = (i) => onChange(experience.filter((_, j) => j !== i))
  const updateExp = (i, field, val) =>
    onChange(experience.map((e, j) => j === i ? { ...e, [field]: val } : e))

  return (
    <div>
      <div className="section-heading">
        <h2>Experience</h2>
        <p>Add internships, jobs, or research positions. All fields except company are optional.</p>
      </div>

      {experience.map((exp, i) => (
        <div className="card" key={i}>
          <div className="card-header">
            <span className="card-title">Experience #{i + 1}</span>
            <button className="btn btn-icon btn-danger" onClick={() => removeExp(i)}>
              <Icon name="trash" size={14}/>
            </button>
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
            <div className="form-grid">
              <Field label="Company" error={errors[`exp_${i}_company`]}>
                <Input
                  value={exp.company}
                  onChange={e => updateExp(i, 'company', e.target.value)}
                  placeholder="e.g. Google"
                  className={errors[`exp_${i}_company`] ? 'error' : ''}
                />
              </Field>
              <Field label="Role / Title" optional>
                <Input
                  value={exp.role}
                  onChange={e => updateExp(i, 'role', e.target.value)}
                  placeholder="Software Engineering Intern"
                />
              </Field>
              <Field label="Duration" optional hint="e.g. May 2024 – Aug 2024">
                <Input
                  value={exp.duration}
                  onChange={e => updateExp(i, 'duration', e.target.value)}
                  placeholder="May 2024 – Aug 2024"
                />
              </Field>
            </div>
            <Field label="Bullet Points" optional>
              <BulletList
                bullets={exp.bullets}
                onChange={v => updateExp(i, 'bullets', v)}
                placeholder="Describe your contributions…"
              />
            </Field>
          </div>
        </div>
      ))}

      <div className="add-entry-row">
        <div className="divider"/>
        <button className="btn btn-secondary btn-sm" onClick={addExp}>
          <Icon name="plus" size={14}/> Add experience
        </button>
        <div className="divider"/>
      </div>
    </div>
  )
}

// ─── Step 6: Extra Sections ──────────────────────────────────
function StepExtras({ extras, onChange }) {
  const addSection = () => onChange([...extras, { title: '', items: [] }])
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

// ─── Result Screen ───────────────────────────────────────────
function ResultScreen({ result, onReset }) {
  const [showDsl, setShowDsl] = useState(false)
  const pdfUrl = result.pdfUrl

  const handleDownload = () => {
    const a = document.createElement('a')
    a.href = pdfUrl
    a.download = result.pdfFilename
    a.click()
  }

  return (
    <div className="result-screen">
      <div className="alert alert-success">
        <span className="alert-icon">✓</span>
        <div>
          <strong>Resume generated successfully!</strong>
          <div style={{ marginTop: 4, fontSize: '0.82rem', opacity: 0.85 }}>
            Saved as <code style={{ fontFamily: 'var(--font-mono)', background: 'rgba(0,0,0,0.08)', padding: '1px 4px', borderRadius: 3 }}>{result.pdfFilename}</code>
          </div>
        </div>
      </div>

      <div className="result-header">
        <h2 style={{ fontFamily: 'var(--font-serif)' }}>Your Resume</h2>
        <div className="result-actions">
          <button className="btn btn-secondary btn-sm" onClick={() => setShowDsl(v => !v)}>
            <Icon name="code" size={14}/> {showDsl ? 'Hide' : 'View'} DSL
          </button>
          <a className="btn btn-secondary btn-sm" href={pdfUrl} target="_blank" rel="noopener noreferrer">
            <Icon name="ext" size={14}/> Open in tab
          </a>
          <button className="btn btn-primary btn-sm" onClick={handleDownload}>
            <Icon name="dl" size={14}/> Download PDF
          </button>
          <button className="btn btn-ghost btn-sm" onClick={onReset}>
            <Icon name="refresh" size={14}/> Start over
          </button>
        </div>
      </div>

      {showDsl && (
        <div>
          <div style={{ marginBottom: '0.5rem', fontSize: '0.8rem', fontWeight: 600, color: 'var(--text-muted)', textTransform: 'uppercase', letterSpacing: '0.05em' }}>
            Generated .resume DSL
          </div>
          <pre className="dsl-preview">{result.dslContent}</pre>
        </div>
      )}

      <div className="pdf-preview-container">
        <iframe
          src={`${pdfUrl}#toolbar=1&view=FitH`}
          title="Resume PDF Preview"
        />
      </div>
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
  const [step, setStep] = useState(0)
  const [formData, setFormData] = useState(INITIAL)
  const [errors, setErrors] = useState({})
  const [loading, setLoading] = useState(false)
  const [result, setResult] = useState(null)
  const [apiError, setApiError] = useState(null)

  const visibleSteps = STEPS.slice(0, 6) // exclude result from bar

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
      }
      const res = await fetch('/api/generate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      })
      const data = await res.json()
      if (!res.ok || !data.success) {
        throw new Error(data.error || 'Generation failed')
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
    setStep(0)
  }

  return (
    <div className="app-shell">
      {/* Header */}
      <header className="app-header">
        <div className="app-header__wordmark">
          <Icon name="doc" size={18}/>
          resume<span>c</span>
          <span className="app-header__badge">compiler</span>
        </div>
        {step < 6 && (
          <span style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
            Step {step + 1} of 6
          </span>
        )}
      </header>

      {loading && (
        <div className="loading-overlay">
          <div className="spinner"/>
          <div className="loading-text">Compiling your resume…</div>
          <p style={{ fontSize: '0.8rem', color: 'var(--text-faint)' }}>
            Running lex → parse → AST → semantic → codegen → pdflatex
          </p>
        </div>
      )}

      <main className="main-content">
        {step === 6 && result ? (
          <ResultScreen result={result} onReset={reset} />
        ) : (
          <>
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
                  <><Icon name="doc" size={16}/> Generate Resume</>
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
