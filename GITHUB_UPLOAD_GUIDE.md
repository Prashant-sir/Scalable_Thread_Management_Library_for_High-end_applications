# Upload Thread-Library to GitHub - Complete Guide

## Prerequisites
- Git installed on your system: https://git-scm.com/download
- GitHub account: https://github.com/signup

## Step-by-Step Instructions

### Step 1: Install Git (if not already installed)
**Windows:**
- Download from: https://git-scm.com/download/win
- Run the installer and follow the default options
- Verify installation: Open Command Prompt and run
  ```
  git --version
  ```

### Step 2: Configure Git Locally
Open **Command Prompt** or **PowerShell** and run:
```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

Replace with your actual name and email associated with your GitHub account.

### Step 3: Create a Repository on GitHub
1. Go to https://github.com/new
2. Enter **Repository name**: `thread-library` (or your preferred name)
3. Choose **Public** or **Private** (public is good for open source)
4. **DO NOT** initialize with README (we already have one)
5. Click **Create repository**
6. Copy the **HTTPS URL** shown or use your repository URL below:
   `https://github.com/Prashant-sir/Scalable_Thread_Management_Library_for_High-end_applications.git`

### Step 4: Initialize Local Git Repository
Open **Command Prompt** and navigate to your project folder:
```bash
cd "c:\Users\PRASHANT KUMAR\Downloads\Thread_LIB_POOL_C_Ubuntu\thread-pool-c-pthreads\assets\thread-library"
```

Then initialize Git:
```bash
git init
```

### Step 5: Add Files to Git
```bash
git add .
```

This stages all files (respecting `.gitignore`).

### Step 6: Create Initial Commit
```bash
git commit -m "Initial commit: Thread pool library with examples and tests"
```

### Step 7: Add Remote Repository
Use your repository URL:
```bash
git remote add origin https://github.com/Prashant-sir/Scalable_Thread_Management_Library_for_High-end_applications.git
```

Verify it was added:
```bash
git remote -v
```

### Step 8: Rename Branch to Main (Optional but Recommended)
```bash
git branch -M main
```

### Step 9: Push to GitHub
```bash
git push -u origin main
```

**If prompted for credentials:**
- Use your GitHub username
- Use a **Personal Access Token** instead of password (recommended):
  1. Go to https://github.com/settings/tokens/new
  2. Select scopes: `repo` (full control of private repositories)
  3. Generate and copy the token
  4. Paste when prompted for password

### Step 10: Verify Upload
Visit: `https://github.com/Prashant-sir/Scalable_Thread_Management_Library_for_High-end_applications`

You should see your code, `.gitignore`, and `README.md` on GitHub!

---

## Common Git Commands for Future Updates

```bash
# Check status
git status

# Add specific file
git add filename.c

# Commit changes
git commit -m "Description of changes"

# Push changes
git push origin main

# Pull latest changes
git pull origin main
```

---

## Troubleshooting

**Q: "fatal: not a git repository"**
- Run `git init` in your project directory

**Q: "fatal: could not read Username"**
- Use Personal Access Token instead of password
- Create one at: https://github.com/settings/tokens/new

**Q: "error: src refspec main does not match any"**
- Ensure you created at least one commit: `git commit -m "message"`

**Q: Large files being rejected**
- GitHub has a 100MB file limit
- Smaller files are ignored by `.gitignore` ✓

---

## What's Included in Your Push

**Files that WILL be pushed:**
- ✓ `src/` - Source code
- ✓ `include/` - Header files
- ✓ `tests/` - Test files
- ✓ `examples/` - Example code
- ✓ `Makefile`
- ✓ `README.md`
- ✓ `.gitignore`

**Files that WON'T be pushed (ignored by `.gitignore`):**
- ✗ `build/` - Compiled binaries
- ✗ `*.o`, `*.a` - Object files
- ✗ `.vscode/`, `.idea/` - IDE settings
- ✗ `*.cbp`, `*.layout` - CodeBlocks IDE files

---

## Next Steps (Optional Enhancements)

1. **Add License**: Create `LICENSE` file (MIT, Apache 2.0, etc.)
   - GitHub offers license templates when creating repo

2. **Add GitHub Actions CI/CD**: Create `.github/workflows/build.yml` for automatic testing

3. **Add More Documentation**: Create `docs/` folder for API documentation

4. **Create Releases**: Tag versions for stable releases
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

---

**Done!** Your thread-library is now on GitHub! 🎉
