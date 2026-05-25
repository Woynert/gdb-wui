# Generate patch:

Make your changes then generate patch with command (no need to commit):

    git -C ./path/to/repo diff --no-ext-diff HEAD > changes.patch

Then register patch file in fetch-deps.py an rerun.

# Check patch (Optional):

Apply patch:

    git apply changes.patch

Check if patch is already applied; if it exists silently it's a success:

    git apply --reverse --check changes.patch
