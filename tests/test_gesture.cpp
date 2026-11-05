// test_gesture.cpp: unit tests for gesture.h.
// Usage: test_gesture [recording.log]
// The optional recording uses one line per touchpad report:
//   r t=<ms> count=<n> mods=<bits> fg=<class> cur=<class> [id=<n> tip=<0|1> x=<n> y=<n>]
#include "gesture.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { printf("FAILED line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

static const long XMAX = 3209;

struct Run { PadFrame f; PadGesture g{XMAX}; int target = 0; long sum = 0; int events = 0; };

// One hybrid frame: one report per finger, Contact Count only in the first report.
static void Frame(Run& r, bool mods, const PadContact* c, int n)
{
    for (int i = 0; i < n; ++i) {
        if (!r.f.Push(i == 0 ? (unsigned)n : 0, &c[i], 1)) continue;
        int t = 0; long d = 0;
        if (r.g.OnFrame(r.f.Contacts(), r.f.Count(), [&] { return mods; }, &t, &d)) {
            r.target = t; r.sum += d; ++r.events;
        }
    }
}

static void Swipe(Run& r, bool mods, long x, long fromY, long toY)
{
    int steps = 10;
    for (int s = 0; s <= steps; ++s) {
        long y = fromY + (toY - fromY) * s / steps;
        PadContact c[2] = { {0, true, x, y}, {1, true, x + 200, y} };
        Frame(r, mods, c, 2);
    }
}

static void Lift(Run& r)
{
    PadContact c[2] = { {0, false, 0, 0}, {1, false, 0, 0} };
    Frame(r, true, c, 2);
}

static void UnitLevelGesture()
{
    { Run r; Swipe(r, true, 400, 1500, 500);  CHECK(r.target == 1); CHECK(r.sum > 900); }   // left, up
    { Run r; Swipe(r, true, 400, 500, 1500);  CHECK(r.target == 1); CHECK(r.sum < -900); }  // left, down
    { Run r; Swipe(r, true, 2600, 1500, 500); CHECK(r.target == 2); CHECK(r.sum > 900); }   // right, up
    { Run r; Swipe(r, true, 2600, 500, 1500); CHECK(r.target == 2); CHECK(r.sum < -900); }  // right, down
    { Run r; Swipe(r, false, 400, 1500, 500); CHECK(r.events == 0); }                       // keys not held
    { Run r; for (int y = 1500; y > 500; y -= 100) {                                        // one finger
          PadContact c = {0, true, 400, y}; Frame(r, true, &c, 1); }
      CHECK(r.events == 0); }
    { Run r; for (int y = 1500; y > 500; y -= 100) {                                        // three fingers
          PadContact c[3] = { {0,true,400,y}, {1,true,600,y}, {2,true,800,y} }; Frame(r, true, c, 3); }
      CHECK(r.events == 0); }
    { Run r; for (int i = 0; i < 20; ++i) {                                                 // resting fingers tremble
          long y = 1000 + (i % 2) * 10;
          PadContact c[2] = { {0,true,400,y}, {1,true,600,y} }; Frame(r, true, c, 2); }
      CHECK(r.events == 0); }
    { Run r; Swipe(r, true, 400, 1500, 1000);                                               // the side stays locked
      int before = r.target;
      for (int s = 0; s <= 10; ++s) {
          long y = 1000 - s * 30, x = 400 + s * 250;
          PadContact c[2] = { {0,true,x,y}, {1,true,x+200,y} }; Frame(r, true, c, 2); }
      CHECK(before == 1); CHECK(r.target == 1); }
    { Run r; Swipe(r, true, 400, 1500, 1000); Lift(r);                                      // lift, then change side
      Swipe(r, true, 2600, 1500, 1000); CHECK(r.target == 2); }
    { Run r; Swipe(r, true, 400, 1500, 1000);                                               // keys released midway
      int ev = r.events; Swipe(r, false, 400, 1000, 500); CHECK(r.events == ev); }
    { PadFrame f; PadContact c = {1, true, 400, 500};                                       // continuation without a start
      CHECK(!f.Push(0, &c, 1)); }
    { PadFrame f; PadGesture g(XMAX); int t = 0; long d = 0, sum = 0;                       // two fingers in one report
      for (long y = 1500; y >= 500; y -= 100) {
          PadContact c[2] = { {0,true,400,y}, {1,true,600,y} };
          CHECK(f.Push(2, c, 2));
          if (g.OnFrame(f.Contacts(), f.Count(), [] { return true; }, &t, &d)) sum += d; }
      CHECK(t == 1); CHECK(sum > 900); }
    { PadFrame f; PadGesture g(XMAX); int calls = 0, t = 0; long d = 0;                     // keys not read for one finger
      PadContact c = {0, true, 400, 500};
      CHECK(f.Push(1, &c, 1));
      g.OnFrame(f.Contacts(), f.Count(), [&] { ++calls; return true; }, &t, &d);
      CHECK(calls == 0); }
}

struct MoveRun {
    MoveGesture g;
    unsigned long long now = 1000;
    double x = 0, y = 0;
    int started = 0, moved = 0, ended = 0;

    void Frame(int fingers, double px, double py, int droppedId = -1)
    {
        PadContact c[5];
        int n = 0;
        for (int i = 0; i < fingers; ++i) {
            if (i == droppedId) continue;
            c[n++] = PadContact{(unsigned)i, true, (long)(400 + i * 500 + px), (long)(1000 + py)};
        }
        double dx, dy;
        MoveGesture::Result res = g.OnFrame(c, n, now, &dx, &dy);
        now += 10;
        x += dx;
        y += dy;
        if (res == MoveGesture::STARTED) ++started;
        if (res == MoveGesture::MOVED) ++moved;
        if (res == MoveGesture::ENDED) ++ended;
    }
    void Drag(int fingers, double fromX, double toX, int steps = 20)
    {
        for (int s = 0; s <= steps; ++s) Frame(fingers, fromX + (toX - fromX) * s / steps, 0);
    }
};

static void UnitMoveGesture()
{
    { MoveRun r; r.Drag(5, 0, 600); r.Frame(0, 0, 0);                                       // five-finger swipe
      CHECK(r.started == 1); CHECK(r.ended == 1); CHECK(fabs(r.x - 600) < 2); CHECK(fabs(r.y) < 1); }
    { MoveRun r; for (int f = 1; f <= 4; ++f) r.Drag(f, 0, 600);                            // 1 to 4 fingers never trigger
      CHECK(r.started == 0); CHECK(r.moved == 0); CHECK(r.x == 0); }
    { MoveRun r; for (int s = 0; s < 40; ++s) r.Frame(5, s % 2 ? 8 : 0, 0);                 // resting fingers tremble
      CHECK(r.started == 0); }
    { MoveRun r; r.Drag(5, 0, 200);                                                         // a finger slips off briefly: no jump
      double before = r.x; r.Frame(5, 200, 0, 2); r.Frame(5, 210, 0, 2); r.Frame(5, 220, 0, 2);
      r.Frame(5, 230, 0); CHECK(r.ended == 0); CHECK(fabs(r.x - before - 30) < 2); }
    { MoveRun r; r.Drag(5, 0, 200);                                                         // four fingers for too long: ends
      for (int s = 0; s < 30; ++s) r.Frame(5, 200, 0, 0);
      CHECK(r.ended == 1); }
    { MoveRun r; r.Drag(5, 0, 200); double before = r.x;                                    // fingers lift one by one: no jump
      r.Frame(4, 200, 0); r.Frame(3, 200, 0); r.Frame(2, 200, 0); r.Frame(1, 200, 0); r.Frame(0, 0, 0);
      CHECK(r.ended == 1); CHECK(fabs(r.x - before) < 1); }
    { MoveRun r; r.Drag(5, 0, 200); r.Frame(0, 0, 0); r.Drag(5, 0, 200); r.Frame(0, 0, 0);  // a second gesture
      CHECK(r.started == 2); CHECK(r.ended == 2); CHECK(fabs(r.x - 400) < 3); }
    { MoveRun r;                                                                            // slow movement is not rounded away
      for (int s = 0; s <= 600; ++s) r.Frame(5, s * 0.2 + (s % 5), 0);
      CHECK(r.started == 1); CHECK(r.x > 100); }
}

static void Replay(const char* path, bool useMods)
{
    FILE* f = fopen(path, "r");
    if (!f) { printf("cannot open %s\n", path); ++g_fail; return; }
    PadFrame frame;
    PadGesture g(XMAX);
    MoveGesture move;
    static char line[512];
    long reports = 0, events = 0, side[3] = {0, 0, 0}, moveEvents = 0;
    std::map<std::string, long> byWindow;
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "r ", 2)) continue;
        unsigned long tick; int count, mods; char fg[64], cur[64];
        if (sscanf(line, "r t=%lu count=%d mods=%d fg=%63s cur=%63s", &tick, &count, &mods, fg, cur) != 5) continue;
        const char* b = strchr(line, '[');
        if (!b) continue;
        PadContact c; int tip = 0;
        if (sscanf(b, "[id=%u tip=%d x=%ld y=%ld]", &c.id, &tip, &c.x, &c.y) != 4) continue;
        c.tip = tip != 0;
        ++reports;
        bool down = useMods && (mods & 5) == 5;
        if (!frame.Push(count < 0 ? 0 : (unsigned)count, &c, 1)) continue;
        int t = 0; long d = 0;
        if (g.OnFrame(frame.Contacts(), frame.Count(), [&] { return down; }, &t, &d)) {
            ++events; ++side[t]; byWindow[fg]++;
            CHECK(down);
        }
        double mx, my;
        if (move.OnFrame(frame.Contacts(), frame.Count(), tick, &mx, &my) != MoveGesture::NONE) ++moveEvents;
    }
    fclose(f);
    printf("replay %s: reports=%ld events=%ld brightness=%ld volume=%ld move=%ld\n",
           useMods ? "with keys" : "without keys", reports, events, side[1], side[2], moveEvents);
    for (auto& kv : byWindow) printf("   %s: %ld events\n", kv.first.c_str(), kv.second);
    if (!useMods) CHECK(events == 0);
}

int main(int argc, char** argv)
{
    UnitLevelGesture();
    UnitMoveGesture();
    if (argc > 1) { Replay(argv[1], true); Replay(argv[1], false); }
    if (g_fail) printf("RESULT: %d FAILED\n", g_fail);
    else printf("RESULT: all passed\n");
    return g_fail ? 1 : 0;
}
