// Wasteland Infinite additions. See LICENSE.md.
#ifndef WASTELAND_RUNTIME_H
#define WASTELAND_RUNTIME_H
namespace wasteland {
// -1: continue normal startup; otherwise exit with this status.
int preflight(int argc, char** argv);
void populateNewGame();
bool nativeProbeRequested();
bool placementSucceeded();
void resetPlacementStatus();
bool layoutGameplayRequested();
bool skipNewGameIntro();
} // namespace wasteland
#endif
