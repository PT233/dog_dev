# Secret handling

Keep API keys in your local environment or an ignored `.env` file. Commit only
placeholder values in `.env.example`. Never include keys in source, logs, or
generated build files.

Install the existing pre-commit checks before committing:

```sh
python -m pip install pre-commit
pre-commit install
```

The Gitleaks hook scans staged changes for credentials, including force-added
files from otherwise excluded build directories. Its output redacts secrets.

Colcon/CMake environment snapshots can capture inherited credentials. When a
build does not need an API key, remove it from that build's environment:

```sh
env -u OPENAI_API_KEY colcon build
```

If a key is exposed, revoke it at its provider, create a replacement, and update
only local/deployment secret storage. Removing a file from the current branch
does not revoke its key or erase older commits.
