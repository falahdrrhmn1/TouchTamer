// gesture.h: touchpad gesture logic without Win32, so it can be tested.
#pragma once

struct PadContact {
    unsigned id;
    bool tip;
    long x, y;
};

// A touchpad can split one frame across several reports (hybrid mode):
// the first report carries the Contact Count, the following ones carry 0.
class PadFrame {
public:
    enum { MAX_CONTACTS = 16 };

    // Returns true when a frame is complete and can be read with Contacts() and Count().
    bool Push(unsigned contactCount, const PadContact* c, int n)
    {
        if (contactCount > 0) {
            expected_ = contactCount;
            got_ = 0;
            inFrame_ = true;
        }
        if (!inFrame_) return false;
        for (int i = 0; i < n && got_ < MAX_CONTACTS; ++i) frame_[got_++] = c[i];
        if ((unsigned)got_ < expected_) return false;
        inFrame_ = false;
        return true;
    }

    const PadContact* Contacts() const { return frame_; }
    int Count() const { return got_; }

private:
    PadContact frame_[MAX_CONTACTS];
    int got_ = 0;
    unsigned expected_ = 0;
    bool inFrame_ = false;
};

// Modifier keys + two fingers: left half of the touchpad = brightness, right half = volume.
class PadGesture {
public:
    enum { DEADZONE = 30 };   // in touchpad units

    explicit PadGesture(long xMax = 0) : xMax_(xMax) {}
    void SetXMax(long xMax) { xMax_ = xMax; }
    int ActiveTarget() const { return active_ ? target_ : 0; }

    // Returns true when something changed. target 1 = brightness, 2 = volume; dy > 0 = fingers moved up.
    // modsDown() is called only while two fingers are down.
    template <class ModsDown>
    bool OnFrame(const PadContact* c, int n, ModsDown modsDown, int* target, long* dy)
    {
        int tips = 0;
        long sumX = 0, sumY = 0;
        unsigned idA = 0, idB = 0;
        for (int i = 0; i < n; ++i) {
            if (!c[i].tip) continue;
            if (tips == 0) idA = c[i].id;
            else if (tips == 1) idB = c[i].id;
            sumX += c[i].x;
            sumY += c[i].y;
            ++tips;
        }
        if (tips != 2 || !modsDown()) {
            active_ = false;
            return false;
        }
        long avgX = sumX / 2, avgY = sumY / 2;
        if (!active_ || idA != idA_ || idB != idB_) {   // new gesture: the side is locked here
            active_ = true;
            moving_ = false;
            idA_ = idA;
            idB_ = idB;
            target_ = (avgX * 2 < xMax_) ? 1 : 2;
            lastY_ = avgY;
            return false;
        }
        long d = lastY_ - avgY;
        if (!moving_) {
            if (d > -DEADZONE && d < DEADZONE) return false;   // ignore fingers that only tremble
            moving_ = true;
        }
        lastY_ = avgY;
        if (d == 0) return false;
        *target = target_;
        *dy = d;
        return true;
    }

private:
    long xMax_;
    bool active_ = false;
    bool moving_ = false;
    unsigned idA_ = 0, idB_ = 0;
    int target_ = 0;
    long lastY_ = 0;
};

// Five fingers: move a window. A finger that slips off for a moment does not end the gesture.
class MoveGesture {
public:
    enum Result { NONE, STARTED, MOVED, ENDED };
    enum { FINGERS = 5, KEEP_FINGERS = 4, GRACE_MS = 200 };
    static constexpr long DEADZONE_SQ = 40 * 40;

    // dx, dy are in touchpad units, averaged over the fingers also present in the previous frame.
    Result OnFrame(const PadContact* c, int n, unsigned long long nowMs, double* dx, double* dy)
    {
        *dx = *dy = 0;
        int tips = 0;
        long sumX = 0, sumY = 0;
        int matched = 0;
        PadContact cur[PadFrame::MAX_CONTACTS];
        for (int i = 0; i < n; ++i) {
            if (!c[i].tip) continue;
            for (int k = 0; k < prevN_; ++k) {
                if (prev_[k].id == c[i].id) {
                    sumX += c[i].x - prev_[k].x;
                    sumY += c[i].y - prev_[k].y;
                    ++matched;
                    break;
                }
            }
            cur[tips++] = c[i];
        }
        for (int i = 0; i < tips; ++i) prev_[i] = cur[i];
        prevN_ = tips;

        double mx = matched ? (double)sumX / matched : 0;
        double my = matched ? (double)sumY / matched : 0;
        if (tips == FINGERS) lastFull_ = nowMs;

        switch (state_) {
        case IDLE:
            if (tips == FINGERS) {
                state_ = ARMED;
                armedX_ = armedY_ = 0;
            }
            return NONE;
        case ARMED:
            if (tips != FINGERS) {
                state_ = IDLE;
                return NONE;
            }
            armedX_ += mx;
            armedY_ += my;
            if (armedX_ * armedX_ + armedY_ * armedY_ < 1.0 * DEADZONE_SQ) return NONE;
            state_ = MOVING;
            *dx = armedX_;
            *dy = armedY_;
            return STARTED;
        case MOVING:
            if (tips == 0 || (tips < FINGERS && nowMs - lastFull_ > GRACE_MS)) {
                state_ = IDLE;
                return ENDED;
            }
            if (tips < KEEP_FINGERS || (mx == 0 && my == 0)) return NONE;
            *dx = mx;
            *dy = my;
            return MOVED;
        }
        return NONE;
    }

private:
    enum State { IDLE, ARMED, MOVING };
    State state_ = IDLE;
    PadContact prev_[PadFrame::MAX_CONTACTS];
    int prevN_ = 0;
    unsigned long long lastFull_ = 0;
    double armedX_ = 0, armedY_ = 0;
};
