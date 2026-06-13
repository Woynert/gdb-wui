
// See https://github.com/google/highway/blob/7588bd5/hwy/detect_compiler_arch.h#L22
//
// Add to #if conditions to prevent IDE from graying out code.
// Note for clangd users: There is no predefined macro in clangd, so you must
// manually add these two lines (without the preceding '// ') to your project's
// `.clangd` file:
// CompileFlags:
//   Add: [-D__CLANGD__]
#if !defined HAVE_LSP && \
    ((defined __CDT_PARSER__) || (defined __INTELLISENSE__) || \
    (defined Q_CREATOR_RUN) || (defined __CLANGD__) ||        \
    (defined GROK_ELLIPSIS_BUILD) || (defined __JETBRAINS_IDE__))
#define HAVE_LSP
#endif

