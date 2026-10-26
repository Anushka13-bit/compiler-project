#ifndef JOB_MATCHER_H
#define JOB_MATCHER_H

#include <stdbool.h>
#include <stddef.h>
#include "ast.h"

/* Structure representing a match analysis between a Resume and a Job */
typedef struct {
    char *source_info;        /* URL, file path, or "Direct Job Input" */
    char *raw_text;           /* Clean text of the job description */
    
    char **job_skills;        /* Skills required by the job */
    int job_skill_count;
    int job_skill_capacity;

    char **matched_skills;    /* Skills present in both job and resume */
    int matched_count;
    int matched_capacity;

    char **missing_skills;    /* Skills required by job but absent from resume */
    int missing_count;
    int missing_capacity;

    double match_percentage;  /* (matched / total) * 100 */
} JobAnalysis;

/*
 * Fetches/loads the job description text.
 * Supports:
 * - Web URLs: starts with "http://" or "https://" (fetches via curl and strips HTML)
 * - Local files: readable text/HTML file path
 * - Direct text: if not a URL or file, returns duplicate of the string
 * Returns dynamically allocated string, or NULL on error.
 */
char *job_fetch_description(const char *source);

/*
 * Analyzes a resume AST against the given job description text.
 * Returns a newly allocated JobAnalysis struct (free with job_analysis_free).
 */
JobAnalysis *job_analyze_resume(const Resume *r, const char *job_text, const char *source_info);

/*
 * Checks if a specific skill name (or its common synonyms) is mentioned anywhere
 * in the resume (skills section, project titles/bullets, experience, extras).
 */
bool resume_has_skill(const Resume *r, const char *skill_name);

/*
 * Prints an initial Target Job Profile summary banner before resume input.
 */
void job_print_target_profile(const char *source, char **skills, int skill_count);

/*
 * Prints a high-impact terminal compatibility report with colored indicators.
 */
void job_print_report(const JobAnalysis *analysis);

/*
 * Frees memory allocated for a JobAnalysis object.
 */
void job_analysis_free(JobAnalysis *analysis);

#endif /* JOB_MATCHER_H */
