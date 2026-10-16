// Wasteland Infinite additions. See LICENSE.md.
#ifndef WASTELAND_RUNTIME_H
#define WASTELAND_RUNTIME_H
namespace wasteland {
// -1: continue normal startup; otherwise exit with this status.
int preflight(int argc, char** argv);
void populateNewGame();
} // namespace wasteland
#endif
