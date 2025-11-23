# Modifying System Prompts

Learn how to customize Claude's behavior through system prompts using file-based configurations (CLAUDE.md, output styles) and runtime parameters (append, custom).

---

<Tip>
**Quick Start**: Most users should combine **CLAUDE.md** (project standards) with **append** (task-specific instructions). These approaches complement each other rather than being mutually exclusive.
</Tip>

System prompts define Claude's behavior, capabilities, and response style throughout a conversation. The Claude Agent SDK provides flexible ways to customize these prompts, from persistent file-based configurations to runtime session overrides.

## Understanding System Prompts

A system prompt is the initial instruction set that shapes how Claude behaves throughout a conversation.

<Note>
**Default behavior:** The Agent SDK uses an **empty system prompt** by default for maximum flexibility. To use Claude Code's comprehensive system prompt (tool instructions, code guidelines, safety rules), specify:

- **TypeScript**: `systemPrompt: { preset: "claude_code" }`
- **Python**: `system_prompt={"type": "preset", "preset": "claude_code"}`
</Note>

The `claude_code` preset includes:

- **Tool usage instructions** - How to use Read, Write, Edit, Bash, etc.
- **Code style guidelines** - Formatting, conventions, best practices
- **Response tone settings** - Verbosity, emoji usage, markdown formatting
- **Security instructions** - What operations require user approval
- **Environment context** - Working directory, git status, platform info

## Two Dimensions of Customization

Understanding these two independent dimensions helps you choose the right approach:

### Dimension 1: Persistence (Where Instructions Live)

**File-based (Persistent)**
- **CLAUDE.md**: Project or user-level markdown files (`./.claude/CLAUDE.md`, `~/.claude/CLAUDE.md`)
- **Output Styles**: Saved persona configurations (`.claude/output-styles/`)
- **Characteristics**: Survive across sessions, can be version controlled, shareable with team

**Runtime (Session-only)**
- **systemPrompt parameter**: Specified in code when calling `query()`
- **Characteristics**: Temporary, perfect for task-specific adjustments, no file management

### Dimension 2: Scope (How Much You Change)

**Additive (Recommended)**
- **Append to `claude_code`**: Keeps all built-in tools and safety features
- **Use case**: Add your instructions on top of Claude Code's foundation

**Complete Override**
- **Custom system prompt**: Full control, but loses built-in functionality
- **Use case**: Specialized non-coding tasks that don't need file tools

<Note>
**Combining approaches**: You can (and often should) use multiple methods together. For example: Project CLAUDE.md + User CLAUDE.md + Active output style + Runtime append all work in combination.
</Note>

## Customization Approaches

### Approach 1: CLAUDE.md Files (Project Context)

CLAUDE.md files provide persistent, project-specific context and instructions that are automatically discovered by the Agent SDK.

#### How CLAUDE.md Works

**File Locations:**

- **Project-level**: `./CLAUDE.md` or `./.claude/CLAUDE.md` (shared with team via git)
- **User-level**: `~/.claude/CLAUDE.md` (personal preferences only)

<Warning>
**Critical Requirement**: The `claude_code` preset does NOT automatically load CLAUDE.md files.

You must explicitly configure `settingSources`:
- **TypeScript**: `settingSources: ['project']` or `['user']`
- **Python**: `setting_sources=["project"]` or `["user"]`

Without this setting, your CLAUDE.md instructions will be silently ignored.
</Warning>

**Content Format:**
CLAUDE.md files use plain markdown and can contain:

- Coding guidelines and standards
- Project architecture overview
- Common commands and workflows
- API conventions and patterns
- Testing requirements
- Deployment procedures

#### Example CLAUDE.md

```markdown
# Project Guidelines

## Code Style

- Use TypeScript strict mode
- Prefer functional components in React
- Always include JSDoc comments for public APIs
- Max line length: 100 characters

## Testing

- Run `npm test` before committing
- Maintain >80% code coverage
- Use jest for unit tests, playwright for E2E
- Test files: `*.test.ts` (unit), `*.e2e.ts` (integration)

## Common Commands

- **Build**: `npm run build`
- **Dev server**: `npm run dev`
- **Type check**: `npm run typecheck`
- **Lint**: `npm run lint:fix`

## Architecture Notes

- API calls go through `src/api/client.ts`
- State management uses Zustand (no Redux)
- Route handlers are in `src/app/api/`
```

#### Using CLAUDE.md with the SDK

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

const messages = [];

for await (const message of query({
  prompt: "Add a new React component for user profiles",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code", // Use Claude Code's base prompt
    },
    settingSources: ["project", "user"], // Load both project and user CLAUDE.md
  },
})) {
  messages.push(message);
  if (message.type === "assistant") {
    console.log(message.message.content);
  }
}

// Claude now has:
// - Claude Code's tools and guidelines
// - Your project's CLAUDE.md standards
// - Your personal CLAUDE.md preferences (if exists)
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

messages = []

async for message in query(
    prompt="Add a new React component for user profiles",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code"  # Use Claude Code's base prompt
        },
        setting_sources=["project", "user"]  # Load both project and user CLAUDE.md
    )
):
    messages.append(message)
    if message.type == 'assistant':
        print(message.message.content)

# Claude now has:
# - Claude Code's tools and guidelines
# - Your project's CLAUDE.md standards
# - Your personal CLAUDE.md preferences (if exists)
```

</CodeGroup>

#### When to Use CLAUDE.md

**Best for:**

- ✅ **Team-shared context** - Guidelines everyone should follow
- ✅ **Project conventions** - Coding standards, file structure, naming patterns
- ✅ **Common commands** - Build, test, deploy commands specific to your project
- ✅ **Long-term memory** - Context that should persist across all sessions
- ✅ **Version-controlled instructions** - Commit to git so the team stays in sync

**Project vs User level:**

- **Project** (`./CLAUDE.md`): Team standards, shared conventions
- **User** (`~/.claude/CLAUDE.md`): Personal preferences (verbose explanations, preferred patterns)

### Approach 2: Output Styles (Saved Personas)

Output styles are reusable persona configurations stored as markdown files. Think of them as specialized "modes" you can activate for recurring tasks.

#### Creating an Output Style

<CodeGroup>

```typescript TypeScript
import { writeFile, mkdir, access } from "fs/promises";
import { join } from "path";
import { homedir } from "os";

async function fileExists(path: string): Promise<boolean> {
  try {
    await access(path);
    return true;
  } catch {
    return false;
  }
}

async function createOutputStyle(
  name: string,
  description: string,
  prompt: string
) {
  try {
    // User-level: ~/.claude/output-styles
    // Project-level: .claude/output-styles
    const outputStylesDir = join(homedir(), ".claude", "output-styles");

    await mkdir(outputStylesDir, { recursive: true });

    // Validate input
    if (!name || !description || !prompt) {
      throw new Error("Name, description, and prompt are required");
    }

    const fileName = `${name.toLowerCase().replace(/\s+/g, "-")}.md`;
    const filePath = join(outputStylesDir, fileName);

    // Check for existing style
    if (await fileExists(filePath)) {
      console.warn(`⚠️  Output style "${name}" already exists. Overwriting...`);
    }

    const content = `---
name: ${name}
description: ${description}
---

${prompt}`;

    await writeFile(filePath, content, "utf-8");

    console.log(`✓ Created output style: ${name}`);
    console.log(`  Location: ${filePath}`);
    console.log(`  Activate with: /output-style ${name}`);

    return filePath;
  } catch (error) {
    console.error(`Failed to create output style: ${error.message}`);
    throw error;
  }
}

// Example: Create a security-focused code reviewer
await createOutputStyle(
  "Security Reviewer",
  "Security-focused code review specialist",
  `You are a security engineer reviewing code for vulnerabilities.

Priority checks (in order):
1. **Authentication/Authorization**: Token validation, permission checks
2. **Input Validation**: SQL injection, XSS, command injection
3. **Secrets Management**: Hardcoded credentials, logging sensitive data
4. **Cryptography**: Weak algorithms, insecure random, improper key storage
5. **Dependencies**: Known CVEs, outdated packages

Output format:
- 🔴 Critical: Immediate security risk
- 🟡 Warning: Potential vulnerability
- 🟢 Pass: Secure implementation
- 💡 Suggestion: Security enhancement

Always include:
- CWE reference if applicable
- Code example of fix
- Attack scenario explanation`
);
```

```python Python
from pathlib import Path

async def create_output_style(name: str, description: str, prompt: str):
    try:
        # User-level: ~/.claude/output-styles
        # Project-level: .claude/output-styles
        output_styles_dir = Path.home() / '.claude' / 'output-styles'

        output_styles_dir.mkdir(parents=True, exist_ok=True)

        # Validate input
        if not name or not description or not prompt:
            raise ValueError("Name, description, and prompt are required")

        file_name = name.lower().replace(' ', '-') + '.md'
        file_path = output_styles_dir / file_name

        # Check for existing style
        if file_path.exists():
            print(f"⚠️  Output style '{name}' already exists. Overwriting...")

        content = f"""---
name: {name}
description: {description}
---

{prompt}"""

        file_path.write_text(content, encoding='utf-8')

        print(f"✓ Created output style: {name}")
        print(f"  Location: {file_path}")
        print(f"  Activate with: /output-style {name}")

        return str(file_path)
    except Exception as error:
        print(f"Failed to create output style: {str(error)}")
        raise

# Example: Create a security-focused code reviewer
await create_output_style(
    'Security Reviewer',
    'Security-focused code review specialist',
    """You are a security engineer reviewing code for vulnerabilities.

Priority checks (in order):
1. **Authentication/Authorization**: Token validation, permission checks
2. **Input Validation**: SQL injection, XSS, command injection
3. **Secrets Management**: Hardcoded credentials, logging sensitive data
4. **Cryptography**: Weak algorithms, insecure random, improper key storage
5. **Dependencies**: Known CVEs, outdated packages

Output format:
- 🔴 Critical: Immediate security risk
- 🟡 Warning: Potential vulnerability
- 🟢 Pass: Secure implementation
- 💡 Suggestion: Security enhancement

Always include:
- CWE reference if applicable
- Code example of fix
- Attack scenario explanation"""
)
```

</CodeGroup>

#### Using Output Styles

**Activation methods:**

- **CLI**: `/output-style Security Reviewer`
- **Create new**: `/output-style:new Security-focused code reviews`
- **Disable**: `/output-style:off`
- **Settings file**: `.claude/settings.local.json` (persistent)

**In SDK code:**

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

// Output style must be activated via CLI or settings file first
// Then SDK will load it when you include settingSources

const messages = [];

for await (const message of query({
  prompt: "Review this authentication module",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",
      // Add session-specific focus
      append: `
For this review, prioritize:
- OAuth 2.0 compliance
- Token storage security
- Session management
      `,
    },
    settingSources: ["user"], // Loads active output style
  },
})) {
  messages.push(message);
}

// Claude now has:
// - Claude Code's base prompt
// - Security Reviewer output style
// - Session-specific OAuth focus
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

# Output style must be activated via CLI or settings file first
# Then SDK will load it when you include setting_sources

messages = []

async for message in query(
    prompt="Review this authentication module",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",
            # Add session-specific focus
            "append": """
For this review, prioritize:
- OAuth 2.0 compliance
- Token storage security
- Session management
            """
        },
        setting_sources=["user"]  # Loads active output style
    )
):
    messages.append(message)

# Claude now has:
# - Claude Code's base prompt
# - Security Reviewer output style
# - Session-specific OAuth focus
```

</CodeGroup>

#### When to Use Output Styles

**Best for:**

- ✅ **Recurring specialized roles** - Code reviewer, SQL optimizer, documentation writer
- ✅ **Persistent behavior changes** - Active across multiple sessions
- ✅ **Team-shared personas** - Commit to git for team to use
- ✅ **Complex prompt modifications** - Multi-paragraph instructions you don't want in code

**Examples:**

- Code review assistant with specific checklist
- Data science analyst with pandas preferences
- DevOps specialist with infrastructure focus
- Technical writer with documentation standards

### Approach 3: Runtime Append (Task-Specific)

Add session-specific instructions while preserving all `claude_code` functionality.

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

const messages = [];

for await (const message of query({
  prompt: "Add error handling to the payment processing module",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",
      append: `
For this task:
- Use custom PaymentError classes (not generic Error)
- Log failures to Sentry with payment context
- Implement retry logic with exponential backoff (3 attempts max)
- Never log credit card numbers or CVV
      `,
    },
  },
})) {
  messages.push(message);
  if (message.type === "assistant") {
    console.log(message.message.content);
  }
}

// Claude has: All tools + Safety rules + Task-specific instructions
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

messages = []

async for message in query(
    prompt="Add error handling to the payment processing module",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",
            "append": """
For this task:
- Use custom PaymentError classes (not generic Error)
- Log failures to Sentry with payment context
- Implement retry logic with exponential backoff (3 attempts max)
- Never log credit card numbers or CVV
            """
        }
    )
):
    messages.append(message)
    if message.type == 'assistant':
        print(message.message.content)

# Claude has: All tools + Safety rules + Task-specific instructions
```

</CodeGroup>

#### When to Use Append

**Best for:**

- ✅ **Task-specific focus** - One-time instructions for current session
- ✅ **No file overhead** - Quick adjustments without creating files
- ✅ **Dynamic instructions** - Computed from variables or user input
- ✅ **Temporary overrides** - Different behavior for this session only

**Examples:**

- "Focus on performance optimization for this module"
- "Use verbose explanations for this teaching session"
- "Prioritize backwards compatibility"
- "This is for a mobile app, consider touch interactions"

### Approach 4: Custom System Prompt (Full Control)

Provide a complete custom prompt, replacing all defaults.

<Warning>
**Loss of functionality**: Using a custom system prompt (plain string) disables ALL Claude Code built-in features:
- ❌ No file operation tools (Read, Write, Edit)
- ❌ No bash/git tools
- ❌ No code formatting guidelines
- ❌ No safety instructions

Only use this for specialized non-coding tasks. For most cases, use `append` instead.
</Warning>

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

// Custom prompt - no built-in tools
const customPrompt = `You are a SQL query optimization specialist for PostgreSQL 15.

For every query submitted:
1. Analyze the execution plan
2. Identify performance bottlenecks (Sequential Scans, high cost operations)
3. Suggest specific indexes with CREATE INDEX statements
4. Estimate performance improvement percentage
5. Consider trade-offs (write performance vs read performance)

Index naming convention: idx_<table>_<columns>
Always prefer partial indexes when applicable.
Include estimated index size and maintenance overhead.`;

const messages = [];

for await (const message of query({
  prompt: "Optimize: SELECT * FROM users WHERE email LIKE '%@gmail.com' ORDER BY created_at DESC",
  options: {
    systemPrompt: customPrompt, // Plain string = complete override
  },
})) {
  messages.push(message);
  if (message.type === "assistant") {
    console.log(message.message.content);
  }
}

// Claude is now ONLY a SQL optimizer (no file tools, no code editing)
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

# Custom prompt - no built-in tools
custom_prompt = """You are a SQL query optimization specialist for PostgreSQL 15.

For every query submitted:
1. Analyze the execution plan
2. Identify performance bottlenecks (Sequential Scans, high cost operations)
3. Suggest specific indexes with CREATE INDEX statements
4. Estimate performance improvement percentage
5. Consider trade-offs (write performance vs read performance)

Index naming convention: idx_<table>_<columns>
Always prefer partial indexes when applicable.
Include estimated index size and maintenance overhead."""

messages = []

async for message in query(
    prompt="Optimize: SELECT * FROM users WHERE email LIKE '%@gmail.com' ORDER BY created_at DESC",
    options=ClaudeAgentOptions(
        system_prompt=custom_prompt  # Plain string = complete override
    )
):
    messages.append(message)
    if message.type == 'assistant':
        print(message.message.content)

# Claude is now ONLY a SQL optimizer (no file tools, no code editing)
```

</CodeGroup>

#### When to Use Custom System Prompt

**Best for:**

- ✅ **Specialized non-coding tasks** - SQL optimization, data analysis, Q&A
- ✅ **Complete control** - You know exactly what you need
- ✅ **No file operations** - Pure text/data processing
- ✅ **Testing prompt strategies** - Experimenting with new approaches

**Avoid for:**

- ❌ General coding tasks (use `append` instead)
- ❌ When you need file tools
- ❌ When you need safety/security features

## How Approaches Combine

When you use multiple customization approaches together, they combine in a specific order:

### Order of Precedence

```
Final System Prompt =
  1. Base (claude_code preset OR empty)
  + 2. User CLAUDE.md (~/.claude/CLAUDE.md)
  + 3. Project CLAUDE.md (./CLAUDE.md or ./.claude/CLAUDE.md)
  + 4. Active Output Style (~/.claude/output-styles/*.md)
  + 5. Runtime Append (systemPrompt.append)
```

Each layer is **appended** to create the final prompt. The exception is custom `systemPrompt` (plain string), which **replaces all layers**.

### Example: Maximum Combination

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

// Assuming:
// - User CLAUDE.md exists: "I prefer verbose explanations"
// - Project CLAUDE.md exists: "Use TypeScript strict mode"
// - Output style "Security Reviewer" is active

const messages = [];

for await (const message of query({
  prompt: "Review the OAuth2 implementation",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",              // ~500 tokens: Tools + Guidelines
      append: "Focus on PKCE flow compliance", // +50 tokens: Session-specific
    },
    settingSources: ["user", "project"],   // Loads CLAUDE.md + output style
  },
})) {
  messages.push(message);
}

// Final system prompt contains (in order):
// 1. claude_code preset (~500 tokens)
// 2. User CLAUDE.md (~100 tokens)
// 3. Project CLAUDE.md (~200 tokens)
// 4. Security Reviewer output style (~150 tokens)
// 5. Runtime append (~50 tokens)
// Total: ~1000 tokens
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

# Assuming:
# - User CLAUDE.md exists: "I prefer verbose explanations"
# - Project CLAUDE.md exists: "Use TypeScript strict mode"
# - Output style "Security Reviewer" is active

messages = []

async for message in query(
    prompt="Review the OAuth2 implementation",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",                # ~500 tokens: Tools + Guidelines
            "append": "Focus on PKCE flow compliance"  # +50 tokens: Session-specific
        },
        setting_sources=["user", "project"]  # Loads CLAUDE.md + output style
    )
):
    messages.append(message)

# Final system prompt contains (in order):
# 1. claude_code preset (~500 tokens)
# 2. User CLAUDE.md (~100 tokens)
# 3. Project CLAUDE.md (~200 tokens)
# 4. Security Reviewer output style (~150 tokens)
# 5. Runtime append (~50 tokens)
# Total: ~1000 tokens
```

</CodeGroup>

<Note>
**Token Budget**: System prompts are included in EVERY request. Keep combined length under 2000 tokens for optimal performance. Use Claude's tokenizer to measure: https://docs.anthropic.com/en/docs/build-with-claude/token-counting
</Note>

## Decision Guide

### Quick Decision Tree

**Choose your approach based on these questions:**

1. **Are these instructions for the whole team?**
   - Yes → Use **Project CLAUDE.md**
   - No → Continue to #2

2. **Are these your personal preferences?**
   - Yes → Use **User CLAUDE.md**
   - No → Continue to #3

3. **Is this a recurring specialized role/persona?**
   - Yes → Create an **Output Style**
   - No → Continue to #4

4. **Is this just for the current task/session?**
   - Yes → Use **Append**
   - No → Continue to #5

5. **Do you need complete control without any built-in tools?**
   - Yes → Use **Custom System Prompt** (rare)
   - No → Default to **Append**

### Comparison Table

| Feature | CLAUDE.md | Output Styles | Append | Custom |
|---------|-----------|---------------|--------|--------|
| **Setup Complexity** | Low (create file) | Medium (file + CLI) | Low (code only) | Low (code only) |
| **Persistence** | ✓ Project lifetime | ✓ Across sessions | ✗ Session only | ✗ Session only |
| **Version Control** | ✓ With project | ✓ Optional | ✓ With code | ✓ With code |
| **Team Sharing** | ✓ Automatic (git) | ✓ Via git/config | △ Manual (code) | △ Manual (code) |
| **Requires settingSources** | ✓ Yes | ✓ Yes | ✗ No | ✗ No |
| **Keeps claude_code tools** | ✓ Yes | ✓ Yes | ✓ Yes | ✗ **No** |
| **Combines with others** | ✓ Yes | ✓ Yes | ✓ Yes | ✗ Replaces all |
| **Runtime override** | ✗ No | ✗ No | ✓ Yes | ✓ Yes |
| **Typical Size** | 100-300 tokens | 50-200 tokens | 20-100 tokens | Variable |
| **Best for** | Team standards | Personas/roles | Task-specific | Full control |

**Legend**: ✓ = Yes, ✗ = No, △ = Partial/Manual

## Common Patterns

### Pattern 1: Team Standards + Personal Preferences

The most common pattern for team-based development.

**Setup:**
- **Project CLAUDE.md**: Team coding standards (git tracked)
- **User CLAUDE.md**: Your personal style preferences
- **Runtime append**: Task-specific instructions

**Project** `./.claude/CLAUDE.md`:
```markdown
# Team Standards

## Code Style
- Use TypeScript strict mode
- Prefer functional components in React
- Run `pnpm test` before commits

## Conventions
- Follow conventional commits
- API routes in `src/app/api/`
- Components in `src/components/`

## Commands
- Build: `pnpm build`
- Test: `pnpm test`
- Lint: `pnpm lint:fix`
```

**User** `~/.claude/CLAUDE.md`:
```markdown
# Personal Preferences

- I prefer verbose explanations with step-by-step reasoning
- Always show file paths as: `path/to/file.ts:123`
- Use `async/await` over `.then()` chains
- Explain trade-offs when multiple approaches exist
```

**Code**:

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

const messages = [];

for await (const message of query({
  prompt: "Add error handling to the API module",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",
      append: "Focus on network timeout scenarios and retry logic",
    },
    settingSources: ["user", "project"], // Loads both CLAUDE.md files
  },
})) {
  messages.push(message);
}

// Claude now has:
// - Claude Code tools
// - Team's TypeScript/React standards
// - Your preference for verbose explanations
// - Task-specific timeout focus
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

messages = []

async for message in query(
    prompt="Add error handling to the API module",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",
            "append": "Focus on network timeout scenarios and retry logic"
        },
        setting_sources=["user", "project"]  # Loads both CLAUDE.md files
    )
):
    messages.append(message)

# Claude now has:
# - Claude Code tools
# - Team's TypeScript/React standards
# - Your preference for verbose explanations
# - Task-specific timeout focus
```

</CodeGroup>

### Pattern 2: Specialized One-Off Agent

For specialized tasks without file overhead.

**Use case**: One-time SQL optimization without creating persistent files

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

const sqlOptimizer = `You are a PostgreSQL query optimization specialist.

For every query:
1. Analyze EXPLAIN ANALYZE output
2. Identify slow operations (Sequential Scans, high cost)
3. Suggest indexes with CREATE INDEX statements
4. Estimate performance improvement
5. Consider trade-offs (write speed vs read speed)

Conventions:
- Index names: idx_<table>_<columns>
- Prefer partial indexes when appropriate
- Always include index size estimates
- Note maintenance overhead`;

const messages = [];

for await (const message of query({
  prompt: `Optimize this query:

SELECT u.*, p.*
FROM users u
JOIN profiles p ON u.id = p.user_id
WHERE u.email LIKE '%@gmail.com'
ORDER BY u.created_at DESC
LIMIT 100`,
  options: {
    systemPrompt: sqlOptimizer, // Custom, no file needed
  },
})) {
  messages.push(message);
  if (message.type === "assistant") {
    console.log(message.message.content);
  }
}

// Specialized one-time optimization (no coding tools needed)
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

sql_optimizer = """You are a PostgreSQL query optimization specialist.

For every query:
1. Analyze EXPLAIN ANALYZE output
2. Identify slow operations (Sequential Scans, high cost)
3. Suggest indexes with CREATE INDEX statements
4. Estimate performance improvement
5. Consider trade-offs (write speed vs read speed)

Conventions:
- Index names: idx_<table>_<columns>
- Prefer partial indexes when appropriate
- Always include index size estimates
- Note maintenance overhead"""

messages = []

async for message in query(
    prompt="""Optimize this query:

SELECT u.*, p.*
FROM users u
JOIN profiles p ON u.id = p.user_id
WHERE u.email LIKE '%@gmail.com'
ORDER BY u.created_at DESC
LIMIT 100""",
    options=ClaudeAgentOptions(
        system_prompt=sql_optimizer  # Custom, no file needed
    )
):
    messages.append(message)
    if message.type == 'assistant':
        print(message.message.content)

# Specialized one-time optimization (no coding tools needed)
```

</CodeGroup>

### Pattern 3: Output Style + Session Context

For recurring specialized roles with session-specific variations.

**Use case**: Weekly code review sessions with different focus areas

**1. Create persistent output style** (one-time setup):

```typescript
await createOutputStyle(
  "Code Reviewer",
  "Thorough code review assistant",
  `You are an expert code reviewer.

For every code submission:
1. **Bugs & Logic**: Identify errors, edge cases, race conditions
2. **Security**: Check for vulnerabilities (injection, XSS, auth issues)
3. **Performance**: Evaluate algorithmic complexity, memory usage
4. **Maintainability**: Assess readability, naming, structure
5. **Testing**: Suggest test cases and coverage improvements

Output format:
- 🔴 Critical: Must fix before merge
- 🟡 Suggestion: Consider improving
- 🟢 Good: Well implemented
- 💡 Tip: Optional enhancement

Always include:
- Specific code examples for fixes
- Rationale for each comment
- Overall quality score (1-10)`
);
```

**2. Use across multiple sessions** with different focus:

<CodeGroup>

```typescript TypeScript
import { query } from "@anthropic-ai/claude-agent-sdk";

// Activate output style once (persists)
// Via CLI: /output-style Code Reviewer

// Session 1: OAuth implementation review
const messages1 = [];

for await (const message of query({
  prompt: "Review the OAuth2 implementation in auth/oauth.ts",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",
      append: `
This is for a HIPAA-compliant healthcare app.
Extra scrutiny on:
- PHI data exposure in logs
- Audit logging completeness
- Session timeout compliance (15 min max)
- Token storage security
      `,
    },
    settingSources: ["user"], // Loads active Code Reviewer style
  },
})) {
  messages1.push(message);
}

// Session 2: Payment processing review
const messages2 = [];

for await (const message of query({
  prompt: "Review the payment processing module",
  options: {
    systemPrompt: {
      type: "preset",
      preset: "claude_code",
      append: `
This handles credit card payments.
Extra scrutiny on:
- PCI DSS compliance
- No credit card data in logs
- Proper error handling for declined cards
- Idempotency for retry safety
      `,
    },
    settingSources: ["user"], // Same Code Reviewer style, different context
  },
})) {
  messages2.push(message);
}
```

```python Python
from claude_agent_sdk import query, ClaudeAgentOptions

# Activate output style once (persists)
# Via CLI: /output-style Code Reviewer

# Session 1: OAuth implementation review
messages1 = []

async for message in query(
    prompt="Review the OAuth2 implementation in auth/oauth.ts",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",
            "append": """
This is for a HIPAA-compliant healthcare app.
Extra scrutiny on:
- PHI data exposure in logs
- Audit logging completeness
- Session timeout compliance (15 min max)
- Token storage security
            """
        },
        setting_sources=["user"]  # Loads active Code Reviewer style
    )
):
    messages1.append(message)

# Session 2: Payment processing review
messages2 = []

async for message in query(
    prompt="Review the payment processing module",
    options=ClaudeAgentOptions(
        system_prompt={
            "type": "preset",
            "preset": "claude_code",
            "append": """
This handles credit card payments.
Extra scrutiny on:
- PCI DSS compliance
- No credit card data in logs
- Proper error handling for declined cards
- Idempotency for retry safety
            """
        },
        setting_sources=["user"]  # Same Code Reviewer style, different context
    )
):
    messages2.append(message)
```

</CodeGroup>

## Troubleshooting

### My CLAUDE.md isn't being loaded

**Symptoms**: Instructions in CLAUDE.md are ignored, Claude doesn't follow project standards

**Check these common issues:**

**1. Missing `settingSources` configuration:**

<CodeGroup>

```typescript TypeScript
// ❌ WRONG: CLAUDE.md won't load
query({
  options: {
    systemPrompt: { preset: "claude_code" }
    // Missing settingSources!
  }
})

// ✓ CORRECT: CLAUDE.md loads
query({
  options: {
    systemPrompt: { preset: "claude_code" },
    settingSources: ["project"]  // ← Required!
  }
})
```

```python Python
# ❌ WRONG: CLAUDE.md won't load
query(
    options=ClaudeAgentOptions(
        system_prompt={"type": "preset", "preset": "claude_code"}
        # Missing setting_sources!
    )
)

# ✓ CORRECT: CLAUDE.md loads
query(
    options=ClaudeAgentOptions(
        system_prompt={"type": "preset", "preset": "claude_code"},
        setting_sources=["project"]  # ← Required!
    )
)
```

</CodeGroup>

**2. Wrong file location:**

- ✓ Correct: `./CLAUDE.md` or `./.claude/CLAUDE.md` (project root)
- ✓ Correct: `~/.claude/CLAUDE.md` (user home directory)
- ✗ Wrong: `./claude.md` (lowercase - case sensitive on Linux/Mac)
- ✗ Wrong: `./docs/CLAUDE.md` (subdirectory not checked)
- ✗ Wrong: `./.claude/settings.json` (different file)

**3. File permissions:**

Ensure the file is readable:

```bash
# Check file exists and is readable
ls -la ./CLAUDE.md
ls -la ~/.claude/CLAUDE.md

# Fix permissions if needed
chmod 644 ./CLAUDE.md
```

### System prompt seems too long

**Symptoms**: Slow responses, increased costs, occasional context overflow

**Solutions:**

**1. Check combined length:**

Your final system prompt includes:
- Base `claude_code` preset: ~500 tokens
- User CLAUDE.md: ~100-300 tokens
- Project CLAUDE.md: ~100-300 tokens
- Active output style: ~50-200 tokens
- Runtime append: ~20-100 tokens

**Total can exceed 1000+ tokens**

**2. Reduce size:**

```typescript
// Disable output style temporarily
// CLI: /output-style:off

// Or use minimal settingSources
query({
  options: {
    systemPrompt: { preset: "claude_code", append: "..." },
    settingSources: ["project"] // Skip user settings
  }
})
```

**3. Consolidate instructions:**

Move repetitive runtime appends into CLAUDE.md:

```markdown
<!-- Before: Repeated in every query -->
append: "Always use TypeScript strict mode. Run tests before committing."

<!-- After: Once in CLAUDE.md -->
# Project Guidelines
- Use TypeScript strict mode
- Run `pnpm test` before committing
```

**4. Use custom prompt for specialized tasks:**

If you don't need file tools, use a lean custom prompt:

```typescript
query({
  options: {
    systemPrompt: "You are a SQL optimizer. [concise instructions]"
    // No claude_code, no files loaded = minimal tokens
  }
})
```

### Changes to CLAUDE.md aren't taking effect

**Symptoms**: Updated CLAUDE.md but Claude still follows old instructions

**Cause**: The SDK loads CLAUDE.md at initialization, not on every query

**Solutions:**

**1. Restart your application:**

```typescript
// Old session - cached old CLAUDE.md
const oldMessages = await query(...);

// Restart app or create new query session
// New query - loads fresh CLAUDE.md
const newMessages = await query(...);
```

**2. For long-running apps, reinitialize periodically:**

```typescript
async function runWithFreshContext() {
  // Import fresh (if using dynamic imports)
  const { query } = await import("@anthropic-ai/claude-agent-sdk");

  for await (const message of query({...})) {
    // Uses latest CLAUDE.md
  }
}
```

**3. Verify the file was actually saved:**

```bash
# Check last modified time
ls -la CLAUDE.md

# View contents
cat CLAUDE.md
```

### Output style conflicts with CLAUDE.md

**Symptoms**: Conflicting instructions from different sources

**Solution**: Remember the precedence order:

```
claude_code → User CLAUDE.md → Project CLAUDE.md → Output Style → Append
```

Later instructions take priority for conflicts. Use `append` to override:

```typescript
query({
  options: {
    systemPrompt: {
      preset: "claude_code",
      append: "For THIS session, ignore the verbose explanation preference and be concise."
    },
    settingSources: ["user", "project"]
  }
})
```

## Performance & Security

### Performance Considerations

#### Token Costs

System prompts are included in **every request**, affecting cost and latency:

**Typical token sizes:**
- `claude_code` preset: ~500 tokens
- Average CLAUDE.md: 100-300 tokens
- Output style: 50-200 tokens
- Runtime append: 20-100 tokens

**Example cost calculation:**

```
10 queries with combined 1000-token system prompt:
- Input tokens: 10,000 (system) + 5,000 (queries) = 15,000 tokens
- vs. 500-token system prompt: 5,000 + 5,000 = 10,000 tokens
- Difference: 50% more input tokens
```

**Best practice**: Keep combined system prompt under 2000 tokens.

#### Prompt Caching

Anthropic's prompt caching automatically reduces costs for repeated system prompts:

- **Threshold**: Prompts >1024 tokens are cached
- **Cost reduction**: ~90% for cached portions
- **Cache TTL**: 5 minutes
- **How it works**: Stable prefix is cached, only changes are processed

**Optimization strategy**:

Structure your system prompt with stable prefix:

<CodeGroup>

```typescript TypeScript
// ✓ GOOD: Stable guidelines first, dynamic context last
const append = `
${STABLE_CODING_GUIDELINES}  // ← Cached (rarely changes)

---
Session-specific context:   // ← Not cached (changes often)
- Current task: ${taskName}
- Focus areas: ${focusAreas.join(", ")}
- User preference: ${userPreference}
`;

query({ options: { systemPrompt: { preset: "claude_code", append } } })

// ✗ BAD: Dynamic data at start breaks caching
const append = `
Current task: ${taskName}      // ← Changes break cache
Focus: ${focusAreas}

${STABLE_GUIDELINES}           // ← Never cached
`;
```

```python Python
# ✓ GOOD: Stable guidelines first, dynamic context last
append = f"""
{STABLE_CODING_GUIDELINES}  # ← Cached (rarely changes)

---
Session-specific context:   # ← Not cached (changes often)
- Current task: {task_name}
- Focus areas: {", ".join(focus_areas)}
- User preference: {user_preference}
"""

query(options=ClaudeAgentOptions(
    system_prompt={"type": "preset", "preset": "claude_code", "append": append}
))

# ✗ BAD: Dynamic data at start breaks caching
append = f"""
Current task: {task_name}      # ← Changes break cache
Focus: {focus_areas}

{STABLE_GUIDELINES}            # ← Never cached
"""
```

</CodeGroup>

### Security Considerations

#### Avoid Sensitive Data in CLAUDE.md

<Warning>
**Security risk**: CLAUDE.md files may be:
- Committed to version control (public repos)
- Shared with team members
- Included in SDK telemetry (if enabled)
- Sent to Anthropic's API

**Never include**:
- API keys, passwords, or credentials
- Internal URLs or IP addresses
- Proprietary algorithms or trade secrets
- Customer data or PII examples
- Database connection strings
</Warning>

**Examples:**

```markdown
<!-- ❌ DANGEROUS -->
# Project Config
- API Key: sk-ant-1234567890abcdef
- Database: postgres://user:pass@internal-db.company.com
- Customer email example: john.doe@realcustomer.com

<!-- ✓ SAFE -->
# Project Config
- API keys are in .env file (never commit)
- Database connection via DATABASE_URL environment variable
- Use fake data for examples: user@example.com
```

#### Project vs User Scope

Choose the right scope for your instructions:

**Project-level** (`./.claude/CLAUDE.md`):
- ✓ Coding standards and conventions
- ✓ Build commands and workflows
- ✓ Architecture patterns
- ✓ Public API documentation
- ✗ Personal preferences
- ✗ API keys or secrets

**User-level** (`~/.claude/CLAUDE.md`):
- ✓ Personal coding style preferences
- ✓ Verbosity settings
- ✓ Preferred patterns (async/await, etc.)
- ✓ Personal workflow shortcuts
- ✗ Team-wide standards (use project-level)

#### Output Style Security

Output styles can contain sensitive domain knowledge:

```typescript
// ⚠️  Consider: Should this be shared?
await createOutputStyle(
  "Internal Security Reviewer",
  "Reviews code for company-specific vulnerabilities",
  `Check for these COMPANY-SPECIFIC issues:
  - Our custom auth token format: [REDACTED]
  - Known vulnerabilities in our legacy system: [REDACTED]
  - Internal API endpoints: [REDACTED]`
);

// ✓ Better: Generic security review
await createOutputStyle(
  "Security Reviewer",
  "Generic security code review",
  `Check for common vulnerabilities:
  - SQL injection
  - XSS
  - Authentication issues
  - (No company-specific details)`
);
```

## Migration Guide

### From Repeated Append → CLAUDE.md

**Problem**: Duplicating the same instructions across multiple files

**Before** (repeated code):

```typescript
// api/users.ts
query({ options: { systemPrompt: {
  preset: "claude_code",
  append: "Use REST conventions. Return 404 for not found."
}}})

// api/posts.ts
query({ options: { systemPrompt: {
  preset: "claude_code",
  append: "Use REST conventions. Return 404 for not found."
}}})

// api/comments.ts (same thing again!)
query({ options: { systemPrompt: {
  preset: "claude_code",
  append: "Use REST conventions. Return 404 for not found."
}}})
```

**After** (DRY with CLAUDE.md):

Create `./.claude/CLAUDE.md`:

```markdown
# API Conventions

- Use REST conventions for all endpoints
- Return 404 for not found (not 400 or 500)
- Return 201 for successful creation
- Use proper HTTP methods (GET/POST/PUT/DELETE)
```

Update all files:

```typescript
// All files now share convention
query({
  options: {
    systemPrompt: { preset: "claude_code" },
    settingSources: ["project"]  // Loads shared CLAUDE.md
  }
})
```

**Benefits**:
- ✓ Single source of truth
- ✓ Update once, applies everywhere
- ✓ Version controlled with project
- ✓ Reduces code duplication

### From Custom Prompt → Append

**Problem**: Lost built-in tools and safety features

**Before** (no tools):

```typescript
query({
  options: {
    systemPrompt: "You are a Python expert. Write clean, well-documented code."
  }
})

// Problem: Claude can't read/write files, no git tools, no safety features
```

**After** (keeps tools):

```typescript
query({
  options: {
    systemPrompt: {
      preset: "claude_code",
      append: "You are a Python expert. Write clean, well-documented code with type hints and docstrings."
    }
  }
})

// Now has: File tools + Git + Safety + Your instructions
```

**Migration checklist**:
1. Change from plain string to object
2. Set `preset: "claude_code"`
3. Move custom instructions to `append`
4. Test that tools still work (read/write files)

### From No Structure → Output Styles

**Problem**: Need recurring specialized behavior but don't want code overhead

**Before** (repeated custom prompts):

```typescript
// Week 1: Code review session
query({ options: { systemPrompt: LONG_CODE_REVIEW_PROMPT }})

// Week 2: Same prompt copy-pasted
query({ options: { systemPrompt: LONG_CODE_REVIEW_PROMPT }})

// Week 3: Slightly different version (inconsistent)
query({ options: { systemPrompt: SLIGHTLY_DIFFERENT_REVIEW_PROMPT }})
```

**After** (reusable output style):

```typescript
// One-time setup
await createOutputStyle(
  "Code Reviewer",
  "Thorough code review",
  LONG_CODE_REVIEW_PROMPT
);

// Activate once (persists across sessions)
// CLI: /output-style Code Reviewer

// Week 1
query({ options: {
  systemPrompt: { preset: "claude_code", append: "Focus on auth module" },
  settingSources: ["user"]
}})

// Week 2 (same style, different focus)
query({ options: {
  systemPrompt: { preset: "claude_code", append: "Focus on payment module" },
  settingSources: ["user"]
}})

// Week 3 (consistent base, custom context)
query({ options: {
  systemPrompt: { preset: "claude_code", append: "Focus on API endpoints" },
  settingSources: ["user"]
}})
```

**Benefits**:
- ✓ Consistent base behavior
- ✓ No code duplication
- ✓ Easy to update (one file)
- ✓ Can share with team via git

## See Also

- [Output Styles Documentation](https://code.claude.com/docs/en/output-styles) - Complete guide to creating and managing output styles
- [TypeScript SDK Reference](/docs/en/agent-sdk/typescript) - Full TypeScript SDK API documentation
- [Python SDK Reference](/docs/en/agent-sdk/python) - Full Python SDK API documentation
- [Configuration Guide](https://code.claude.com/docs/en/settings) - General configuration and settings
- [Token Counting](https://docs.anthropic.com/en/docs/build-with-claude/token-counting) - How to measure and optimize token usage
- [Prompt Caching](https://docs.anthropic.com/en/docs/build-with-claude/prompt-caching) - Reduce costs with prompt caching
