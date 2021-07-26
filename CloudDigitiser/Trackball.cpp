#include "Trackball.h"

#include <math.h>

#include "trackball.h"

Trackball::Trackball()
{
    mActive = false;
}


// called with the mouse start position and the trackbal dimensions
// note: clicks outside the trackball radius have a different rotation behaviour
// note: values given in window coordinates with raster origin at top left
void Trackball::StartTrackball(int mouseX, int mouseY, int trackballOriginX, int trackballOriginY, int trackballRadius, const Vector3f &up, const Vector3f &out)
{
    mActive = true;
    mTrackballRadius = trackballRadius;
    mStartMouseX = mouseX;
    mStartMouseY = mouseY;
    mTrackballOriginX = trackballOriginX;
    mTrackballOriginY = trackballOriginY;
    mOut = out;
    mUp = up;
    mOut.Normalize();
    mUp.Normalize();
    mLeft = mUp.Cross(mOut);
    mLeft.Normalize();

    double dx = mStartMouseX - mTrackballOriginX;
    double dy = mTrackballOriginY - mStartMouseY;
    double r = sqrt(dx * dx + dy * dy);
    if (r > trackballRadius) mOutsideRadius = true;
    else mOutsideRadius = false;

    mRotation = Quaternion(0, 0, 0, 1);
}

// calculated rotation based on current mouse position
void Trackball::RollTrackballToClick(int mouseX, int mouseY)
{
    if (mouseX == mStartMouseX && mouseY == mStartMouseY)
    {
        mRotation.w = 1;
        mRotation.x = mRotation.y = mRotation.z = 0;
        return;
    }
    Vector3f v1;
    Vector3f v2;
    if (mOutsideRadius == false) // normal behaviour
    {
        // original code gives wrong movement sense in X direction
        // v1 = (mStartMouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mStartMouseY) * mUp + mTrackballRadius * mOut;
        // v2 = (mouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mouseY) * mUp + mTrackballRadius * mOut;
        v1 = -(mStartMouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mStartMouseY) * mUp + mTrackballRadius * mOut;
        v2 = -(mouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mouseY) * mUp + mTrackballRadius * mOut;

    }
    else // rotate around axis coming out of screen
    {
        v1 = (mStartMouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mStartMouseY) * mUp;
        v2 = (mouseX - mTrackballOriginX) * mLeft + (mTrackballOriginY - mouseY) * mUp;
    }

    // cross product will get us the rotation axis
    Vector3f axis = v1.Cross(v2);

    // Use atan for a better angle.  If you use only cos or sin, you only get
    // half the possible angles, and you can end up with rotations that flip around near
    // the poles.

    // cos angle obtained from dot product formula
    // cos(a) = (s . e) / (||s|| ||e||)
    double cosAng = v1.Dot(v2); // (s . e)
    double ls = v1.Magnitude();
    ls = 1. / ls; // 1 / ||s||
    double le = v2.Magnitude();
    le = 1. / le; // 1 / ||e||
    cosAng = cosAng * ls * le;

    // sin angle obtained from cross product formula
    // sin(a) = ||(s X e)|| / (||s|| ||e||)
    double sinAng = axis.Magnitude(); // ||(s X e)||;
    sinAng = sinAng * ls * le;
    double angle = atan2(sinAng, cosAng); // rotations are in radians.

    mRotation = MakeQuaternionFromAxisAngle(axis.x, axis.y, axis.z, ToDegree(angle));
}

