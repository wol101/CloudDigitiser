#ifndef POINTCLOUDSCENENODE_H
#define POINTCLOUDSCENENODE_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <irrlicht.h>
#pragma GCC diagnostic pop

class PointCloud;

class PointCloudSceneNode : public irr::scene::ISceneNode
{
public:
    PointCloudSceneNode(irr::scene::ISceneNode* parent, irr::scene::ISceneManager* mgr, irr::s32 id);
    ~PointCloudSceneNode();

    virtual void OnRegisterSceneNode();

    virtual void render();

    virtual const irr::core::aabbox3d<irr::f32>& getBoundingBox() const;

    virtual irr::u32 getMaterialCount() const;

    virtual irr::video::SMaterial& getMaterial(irr::u32 i );

    // custom initialiser for this scene node
    void initializeVertices(PointCloud *pointCloud, irr::s32 maxPointsToRender);

protected:
    irr::core::aabbox3d<irr::f32> Box;
    irr::video::SMaterial Material;
    irr::video::S3DVertex *Vertices;
    irr::u32 *IndexList;
    irr::u32 NumVertices;
};

#endif // POINTCLOUDSCENENODE_H
