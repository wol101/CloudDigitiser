#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "pipeline/math_3d.h"

class Trackball
{
public:
    Trackball();

    // called with the mouse start position and the trackbal dimensions
    // note: clicks outside the trackball radius have a different rotation behaviour
    // note: values given in window coordinates with raster origin at top left
    void StartTrackball(int mouseX, int mouseY, int trackballOriginX, int trackballOriginY, int trackballRadius, const Vector3f &up, const Vector3f &out);

    // calculated rotation based on current mouse position
    void RollTrackballToClick(int mouseX, int mouseY);

    int GetTrackballRadius() { return mTrackballRadius; }
    bool GetOutsideRadius() { return mOutsideRadius; }

    void SetActive(bool active) { mActive = active; }
    bool GetActive() { return mActive; }

    const Quaternion &GetRotation() { return mRotation; }

protected:

    int mTrackballRadius;
    int mStartMouseX;
    int mStartMouseY;
    int mTrackballOriginX;
    int mTrackballOriginY;
    bool mOutsideRadius;

    Vector3f mLeft;
    Vector3f mUp;
    Vector3f mOut;
    bool mActive;
    Quaternion mRotation;
};

#endif // TRACKBALL_H
