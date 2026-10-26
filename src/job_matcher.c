#include "job_matcher.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* ── Skill Dictionary with Canonical Names & Synonyms ── */

typedef struct {
    const char *canonical;
    const char *synonyms; /* comma-separated alternatives */
} SkillSpec;

static const SkillSpec SKILL_CATALOG[] = {
    /* Languages */
    {"Python", "python,python3"},
    {"C", "c"},
    {"C++", "c++,cpp"},
    {"C#", "c#,c-sharp"},
    {"Java", "java"},
    {"JavaScript", "javascript,js"},
    {"TypeScript", "typescript,ts"},
    {"Go", "go,golang"},
    {"Rust", "rust"},
    {"Ruby", "ruby"},
    {"PHP", "php"},
    {"Swift", "swift"},
    {"Kotlin", "kotlin"},
    {"SQL", "sql"},
    {"HTML/CSS", "html,css"},
    {"Bash / Shell", "bash,shell"},
    {"R", "r language,r programming"},
    {"Scala", "scala"},
    {"Dart", "dart"},

    /* Web & Backend */
    {"React", "react,reactjs,react.js"},
    {"Next.js", "nextjs,next.js"},
    {"Node.js", "node,nodejs,node.js"},
    {"Express.js", "express,expressjs"},
    {"Vue.js", "vue,vuejs,vue.js"},
    {"Angular", "angular,angularjs"},
    {"Django", "django"},
    {"Flask", "flask"},
    {"FastAPI", "fastapi"},
    {"Spring Boot", "spring boot,spring framework"},
    {"ASP.NET", "asp.net,.net core"},
    {"Ruby on Rails", "rails,ruby on rails"},
    {"GraphQL", "graphql"},
    {"REST API", "rest api,restful api,restful"},
    {"Tailwind CSS", "tailwind,tailwindcss"},

    /* Cloud & DevOps */
    {"AWS", "aws,amazon web services"},
    {"Azure", "azure,microsoft azure"},
    {"GCP", "gcp,google cloud"},
    {"Docker", "docker"},
    {"Kubernetes", "kubernetes,k8s"},
    {"Terraform", "terraform"},
    {"CI/CD", "ci/cd,cicd"},
    {"Git", "git,github,gitlab"},
    {"Linux", "linux,unix"},
    {"Ansible", "ansible"},
    {"Jenkins", "jenkins"},
    {"Kafka", "kafka,apache kafka"},
    {"RabbitMQ", "rabbitmq"},

    /* Databases */
    {"PostgreSQL", "postgresql,postgres"},
    {"MySQL", "mysql"},
    {"MongoDB", "mongodb,mongo"},
    {"Redis", "redis"},
    {"SQLite", "sqlite"},
    {"Elasticsearch", "elasticsearch"},
    {"Cassandra", "cassandra"},
    {"DynamoDB", "dynamodb"},

    {"Machine Learning", "machine learning,ml"},
    {"Deep Learning", "deep learning"},
    {"PyTorch", "pytorch"},
    {"TensorFlow", "tensorflow"},
    {"Pandas", "pandas"},
    {"NumPy", "numpy"},
    {"Scikit-Learn", "scikit-learn,sklearn"},
    {"NLP", "natural language processing,nlp"},
    {"Computer Vision", "computer vision,opencv"},
    {"Large Language Models", "llm,large language model,generative ai"},
    {"Apache Spark", "spark,apache spark,pyspark"},
    {"Hadoop", "hadoop,apache hadoop"},
    {"Tableau", "tableau"},
    {"Power BI", "power bi,powerbi"},
    {"Statistics", "statistics,statistical analysis,hypothesis testing,a/b testing"},
    {"Data Pipelines", "data pipelines,data pipeline,etl"},
    {"Snowflake", "snowflake"},
    {"BigQuery", "bigquery"},

    /* Engineering Practices */
    {"Microservices", "microservices,microservice"},
    {"System Design", "system design"},
    {"Agile / Scrum", "agile,scrum"},
    {"Unit Testing", "unit testing,tdd,test-driven"},
    {"Data Structures & Algorithms", "data structures,algorithms"},

    {NULL, NULL}
};

/* ── Helper: Exact Word/Token Matcher ─────────────────── */

static bool text_contains_token(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return false;
    size_t nlen = strlen(needle);

    for (const char *p = haystack; *p; p++) {
        if (strncasecmp(p, needle, nlen) == 0) {
            /* Check left boundary */
            bool left_ok = (p == haystack);
            if (!left_ok) {
                char before = *(p - 1);
                if (!isalnum((unsigned char)before) && before != '_' && before != '+' && before != '#') {
                    left_ok = true;
                }
            }

            /* Check right boundary */
            char after = *(p + nlen);
            bool right_ok = (after == '\0');
            if (!right_ok) {
                if (!isalnum((unsigned char)after) && after != '_' && after != '+' && after != '#') {
                    right_ok = true;
                }
            }

            if (left_ok && right_ok) return true;
        }
    }
    return false;
}

static bool match_skill_in_text(const char *text, const SkillSpec *spec) {
    if (!text || !spec) return false;
    if (text_contains_token(text, spec->canonical)) return true;

    /* Check synonyms */
    char buf[256];
    strncpy(buf, spec->synonyms, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, ",");
    while (token) {
        while (isspace((unsigned char)*token)) token++;
        if (*token && text_contains_token(text, token)) {
            return true;
        }
        token = strtok(NULL, ",");
    }
    return false;
}

/* ── Helper: Strip HTML Tags & Entities ──────────────── */

static char *strip_html_tags(const char *html) {
    if (!html) return NULL;
    size_t len = strlen(html);
    char *out = malloc(len + 1);
    if (!out) return NULL;

    size_t j = 0;
    bool in_tag = false;
    bool in_script = false;
    bool in_style = false;

    for (size_t i = 0; i < len; i++) {
        if (!in_tag && html[i] == '<') {
            if (strncasecmp(&html[i], "<script", 7) == 0) in_script = true;
            else if (strncasecmp(&html[i], "</script", 8) == 0) in_script = false;
            else if (strncasecmp(&html[i], "<style", 6) == 0) in_style = true;
            else if (strncasecmp(&html[i], "</style", 7) == 0) in_style = false;
            in_tag = true;
            if (j > 0 && out[j - 1] != ' ') out[j++] = ' ';
            continue;
        }
        if (in_tag) {
            if (html[i] == '>') {
                in_tag = false;
            }
            continue;
        }
        if (in_script || in_style) {
            continue;
        }

        /* Basic HTML Entity Decoding */
        if (html[i] == '&') {
            if (strncasecmp(&html[i], "&nbsp;", 6) == 0) {
                if (j > 0 && out[j - 1] != ' ') out[j++] = ' ';
                i += 5;
                continue;
            } else if (strncasecmp(&html[i], "&amp;", 5) == 0) {
                out[j++] = '&';
                i += 4;
                continue;
            } else if (strncasecmp(&html[i], "&lt;", 4) == 0) {
                out[j++] = '<';
                i += 3;
                continue;
            } else if (strncasecmp(&html[i], "&gt;", 4) == 0) {
                out[j++] = '>';
                i += 3;
                continue;
            } else if (strncasecmp(&html[i], "&quot;", 6) == 0) {
                out[j++] = '"';
                i += 5;
                continue;
            } else if (strncasecmp(&html[i], "&#39;", 5) == 0) {
                out[j++] = '\'';
                i += 4;
                continue;
            }
        }

        if (isspace((unsigned char)html[i])) {
            if (j > 0 && out[j - 1] != ' ') out[j++] = ' ';
        } else {
            out[j++] = html[i];
        }
    }
    out[j] = '\0';
    return out;
}

/* ── Fetching Logic: URL or Local File ───────────────── */

static char *fetch_url_via_curl(const char *url) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        /* Child process */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        /* Mute stderr */
        FILE *devnull = fopen("/dev/null", "w");
        if (devnull) {
            dup2(fileno(devnull), STDERR_FILENO);
            fclose(devnull);
        }

        execlp("curl", "curl", "-s", "-L", "--max-time", "15",
               "-A", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)",
               url, (char *)NULL);
        _exit(127);
    }

    /* Parent process */
    close(pipefd[1]);

    size_t cap = 65536;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return NULL;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], buf + len, cap - len - 1)) > 0) {
        len += (size_t)bytes_read;
        if (len + 4096 >= cap) {
            cap *= 2;
            char *new_buf = realloc(buf, cap);
            if (!new_buf) {
                free(buf);
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
                return NULL;
            }
            buf = new_buf;
        }
    }
    close(pipefd[0]);
    waitpid(pid, NULL, 0);

    buf[len] = '\0';

    if (len == 0) {
        free(buf);
        return NULL;
    }

    char *clean = strip_html_tags(buf);
    free(buf);
    return clean;
}

char *job_fetch_description(const char *source) {
    if (!source || !*source) return NULL;

    /* Check if it is a Web URL */
    if (strncasecmp(source, "http://", 7) == 0 || strncasecmp(source, "https://", 8) == 0) {
        return fetch_url_via_curl(source);
    }

    /* Check if it is a readable local file */
    FILE *fp = fopen(source, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (sz <= 0) {
            fclose(fp);
            return NULL;
        }

        char *buf = malloc((size_t)sz + 1);
        if (!buf) {
            fclose(fp);
            return NULL;
        }

        size_t n = fread(buf, 1, (size_t)sz, fp);
        buf[n] = '\0';
        fclose(fp);

        if (strstr(buf, "<html") || strstr(buf, "<body") || strstr(buf, "<div")) {
            char *clean = strip_html_tags(buf);
            free(buf);
            return clean;
        }
        return buf;
    }

    /* Otherwise, treat string itself as raw text */
    return strdup(source);
}

/* ── Resume Inspection ───────────────────────────────── */

bool resume_has_skill(const Resume *r, const char *skill_name) {
    if (!r || !skill_name) return false;

    /* Find matching SkillSpec in catalog */
    const SkillSpec *target_spec = NULL;
    for (int i = 0; SKILL_CATALOG[i].canonical != NULL; i++) {
        if (strcasecmp(SKILL_CATALOG[i].canonical, skill_name) == 0) {
            target_spec = &SKILL_CATALOG[i];
            break;
        }
    }

    SkillSpec fallback = {skill_name, ""};
    const SkillSpec *spec = target_spec ? target_spec : &fallback;

    /* 1. Check Skills section */
    for (int i = 0; i < r->skill_count; i++) {
        if (match_skill_in_text(r->skills[i].text, spec)) return true;
    }

    /* 2. Check Projects */
    for (int i = 0; i < r->project_count; i++) {
        if (match_skill_in_text(r->projects[i].name, spec)) return true;
        for (int j = 0; j < r->projects[i].bullet_count; j++) {
            if (match_skill_in_text(r->projects[i].bullets[j], spec)) return true;
        }
    }

    /* 3. Check Experience entries */
    for (int i = 0; i < r->exp_count; i++) {
        if (r->exp_entries[i].company && match_skill_in_text(r->exp_entries[i].company, spec)) return true;
        if (r->exp_entries[i].role && match_skill_in_text(r->exp_entries[i].role, spec)) return true;
        for (int j = 0; j < r->exp_entries[i].bullet_count; j++) {
            if (match_skill_in_text(r->exp_entries[i].bullets[j], spec)) return true;
        }
    }

    /* 4. Check Extra custom sections */
    for (int i = 0; i < r->extra_count; i++) {
        for (int j = 0; j < r->extras[i].item_count; j++) {
            if (match_skill_in_text(r->extras[i].items[j].text, spec)) return true;
            for (int k = 0; k < r->extras[i].items[j].sub_count; k++) {
                if (match_skill_in_text(r->extras[i].items[j].subbullets[k], spec)) return true;
            }
        }
    }

    return false;
}

/* ── Dynamic Array Helpers ───────────────────────────── */

static void add_str_to_list(char ***list, int *count, int *cap, const char *str) {
    if (*count >= *cap) {
        *cap = (*cap == 0) ? 8 : (*cap * 2);
        *list = realloc(*list, (*cap) * sizeof(char *));
    }
    (*list)[(*count)++] = strdup(str);
}

/* ── Core Job Analysis ───────────────────────────────── */

JobAnalysis *job_analyze_resume(const Resume *r, const char *job_text, const char *source_info) {
    if (!job_text) return NULL;

    JobAnalysis *a = calloc(1, sizeof(JobAnalysis));
    if (!a) return NULL;

    a->source_info = strdup(source_info ? source_info : "Job Description");
    a->raw_text = strdup(job_text);

    /* 1. Extract required skills from the job text */
    for (int i = 0; SKILL_CATALOG[i].canonical != NULL; i++) {
        if (match_skill_in_text(job_text, &SKILL_CATALOG[i])) {
            add_str_to_list(&a->job_skills, &a->job_skill_count, &a->job_skill_capacity, SKILL_CATALOG[i].canonical);
        }
    }

    /* 2. Match against candidate resume */
    for (int i = 0; i < a->job_skill_count; i++) {
        const char *skill = a->job_skills[i];
        if (resume_has_skill(r, skill)) {
            add_str_to_list(&a->matched_skills, &a->matched_count, &a->matched_capacity, skill);
        } else {
            add_str_to_list(&a->missing_skills, &a->missing_count, &a->missing_capacity, skill);
        }
    }

    /* 3. Compute score */
    if (a->job_skill_count > 0) {
        a->match_percentage = ((double)a->matched_count / (double)a->job_skill_count) * 100.0;
    } else {
        a->match_percentage = 100.0;
    }

    return a;
}

void job_print_target_profile(const char *source, char **skills, int skill_count) {
    printf("\n");
    printf("\033[1;35m┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\033[0m\n");
    printf("\033[1;35m┃                  🎯 TARGET JOB PROFILE & REQUIREMENTS               ┃\033[0m\n");
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");

    char src_disp[55];
    if (source && strlen(source) > 50) {
        strncpy(src_disp, source, 47);
        strcpy(src_disp + 47, "...");
    } else {
        strcpy(src_disp, source ? source : "Specified Target Job");
    }
    printf("\033[1;35m┃\033[0m Source: \033[1;37m%-58s\033[0m \033[1;35m┃\033[0m\n", src_disp);
    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[1;33mDiscovered Key Requirements (%d):\033[0m%*s\033[1;35m┃\033[0m\n",
           skill_count, (int)(36 - (skill_count >= 10 ? 1 : 0)), "");

    if (skill_count == 0) {
        printf("\033[1;35m┃\033[0m   (No specific tech keywords identified in posting)               \033[1;35m┃\033[0m\n");
    } else {
        printf("\033[1;35m┃\033[0m   ");
        int col = 3;
        for (int i = 0; i < skill_count; i++) {
            int len = (int)strlen(skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;35m┃\033[0m\n\033[1;35m┃\033[0m   ", 69 - col, "");
                col = 3;
            }
            printf("\033[1;37m• %s\033[0m%s", skills[i], (i + 1 < skill_count) ? ", " : "");
            col += len;
        }
        printf("%*s\033[1;35m┃\033[0m\n", 69 - col, "");
    }

    printf("\033[1;35m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[36m💡 Live Suggestions Active: We will guide you to highlight these    \033[1;35m┃\033[0m\n");
    printf("\033[1;35m┃\033[0m \033[36m   skills across your Skills, Projects, and Experience sections!     \033[1;35m┃\033[0m\n");
    printf("\033[1;35m┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\033[0m\n\n");
}

/* ── High-Impact Terminal Report ──────────────────────── */

void job_print_report(const JobAnalysis *a) {
    if (!a) return;

    printf("\n");
    printf("\033[1;36m┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\033[0m\n");
    printf("\033[1;36m┃                 🎯 JOB REQUIREMENTS MATCH ANALYSIS                  ┃\033[0m\n");
    printf("\033[1;36m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");

    /* Truncate source info if too long */
    char src_disp[55];
    if (strlen(a->source_info) > 50) {
        strncpy(src_disp, a->source_info, 47);
        strcpy(src_disp + 47, "...");
    } else {
        strcpy(src_disp, a->source_info);
    }
    printf("\033[1;36m┃\033[0m Target Job: \033[1;37m%-55s\033[0m \033[1;36m┃\033[0m\n", src_disp);

    /* Match Score Bar */
    int bar_width = 25;
    int filled = (int)((a->match_percentage / 100.0) * bar_width);
    if (filled > bar_width) filled = bar_width;
    char bar[128] = {0};
    int bidx = 0;
    for (int i = 0; i < filled; i++) {
        strcpy(bar + bidx, "█");
        bidx += 3;
    }
    for (int i = filled; i < bar_width; i++) {
        strcpy(bar + bidx, "░");
        bidx += 3;
    }

    const char *color_code = (a->match_percentage >= 75.0) ? "\033[1;32m" :
                             (a->match_percentage >= 50.0) ? "\033[1;33m" : "\033[1;31m";

    printf("\033[1;36m┃\033[0m Match Score: %s[%s] %5.1f%%\033[0m (%d of %d skills covered) \033[1;36m┃\033[0m\n",
           color_code, bar, a->match_percentage, a->matched_count, a->job_skill_count);
    printf("\033[1;36m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");

    /* Matched Skills */
    printf("\033[1;36m┃\033[0m \033[1;32m✓ Matched Skills in Resume (%d):\033[0m%*s\033[1;36m┃\033[0m\n",
           a->matched_count, (int)(38 - (a->matched_count >= 10 ? 1 : 0)), "");
    if (a->matched_count == 0) {
        printf("\033[1;36m┃\033[0m   (No required skills currently matched)                           \033[1;36m┃\033[0m\n");
    } else {
        printf("\033[1;36m┃\033[0m   ");
        int col = 3;
        for (int i = 0; i < a->matched_count; i++) {
            int len = (int)strlen(a->matched_skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;36m┃\033[0m\n\033[1;36m┃\033[0m   ", 69 - col, "");
                col = 3;
            }
            printf("\033[32m•\033[0m %s%s", a->matched_skills[i], (i + 1 < a->matched_count) ? ", " : "");
            col += len;
        }
        printf("%*s\033[1;36m┃\033[0m\n", 69 - col, "");
    }

    /* Missing Skills */
    printf("\033[1;36m┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\033[0m\n");
    printf("\033[1;36m┃\033[0m \033[1;31m⚠️  Missing Skills Mentioned in Job (%d):\033[0m%*s\033[1;36m┃\033[0m\n",
           a->missing_count, (int)(31 - (a->missing_count >= 10 ? 1 : 0)), "");
    if (a->missing_count == 0) {
        printf("\033[1;36m┃\033[0m   \033[1;32m★ Perfect match! All job requirements covered in your resume!\033[0m    \033[1;36m┃\033[0m\n");
    } else {
        printf("\033[1;36m┃\033[0m   ");
        int col = 3;
        for (int i = 0; i < a->missing_count; i++) {
            int len = (int)strlen(a->missing_skills[i]) + 2;
            if (col + len > 66) {
                printf("%*s\033[1;36m┃\033[0m\n\033[1;36m┃\033[0m   ", 69 - col, "");
                col = 3;
            }
            printf("\033[31m✕\033[0m %s%s", a->missing_skills[i], (i + 1 < a->missing_count) ? ", " : "");
            col += len;
        }
        printf("%*s\033[1;36m┃\033[0m\n", 69 - col, "");
    }

    printf("\033[1;36m┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\033[0m\n\n");
}

/* ── Cleanup ─────────────────────────────────────────── */

void job_analysis_free(JobAnalysis *a) {
    if (!a) return;
    free(a->source_info);
    free(a->raw_text);

    for (int i = 0; i < a->job_skill_count; i++) free(a->job_skills[i]);
    free(a->job_skills);

    for (int i = 0; i < a->matched_count; i++) free(a->matched_skills[i]);
    free(a->matched_skills);

    for (int i = 0; i < a->missing_count; i++) free(a->missing_skills[i]);
    free(a->missing_skills);

    free(a);
}
