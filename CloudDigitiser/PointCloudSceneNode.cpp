#include "PointCloudSceneNode.h"
#include "PointCloud.h"

/*
The parameters of the constructor specify the parent of the scene node,
a pointer to the scene manager, and an id of the scene node.
In the constructor we call the parent class' constructor,
set some properties of the material, and potentially set the
drawing data and the bounding box. However this scene node
will do that elsewhere.
*/

PointCloudSceneNode::PointCloudSceneNode(irr::scene::ISceneNode* parent, irr::scene::ISceneManager* mgr, irr::s32 id)
    : irr::scene::ISceneNode(parent, mgr, id)
{
    Material.Wireframe = false;
    Material.Lighting = false;
    Material.PointCloud = true;

    Vertices = 0;
    IndexList = 0;
    NumVertices = 0;
}

PointCloudSceneNode::~PointCloudSceneNode()
{
    if (Vertices) delete [] Vertices;
    if (IndexList) delete [] IndexList;
}

/*
Before it is drawn, the irr::scene::ISceneNode::OnRegisterSceneNode()
method of every scene node in the scene is called by the scene manager.
If the scene node wishes to draw itself, it may register itself in the
scene manager to be drawn. This is necessary to tell the scene manager
when it should call irr::scene::ISceneNode::render(). For
example, normal scene nodes render their content one after another,
while stencil buffer shadows would like to be drawn after all other
scene nodes. And camera or light scene nodes need to be rendered before
all other scene nodes (if at all). So here we simply register the
scene node to render normally. If we would like to let it be rendered
like cameras or light, we would have to call
SceneManager->registerNodeForRendering(this, SNRT_LIGHT_AND_CAMERA);
After this, we call the actual
irr::scene::ISceneNode::OnRegisterSceneNode() method of the base class,
which simply lets also all the child scene nodes of this node register
themselves.
*/
void PointCloudSceneNode::OnRegisterSceneNode()
{
    if (IsVisible)
        SceneManager->registerNodeForRendering(this);

    ISceneNode::OnRegisterSceneNode();
}

/*
In the render() method most of the interesting stuff happens: The
Scene node renders itself. We override this method and draw the
tetraeder.
*/
void PointCloudSceneNode::render()
{
    irr::video::IVideoDriver* driver = SceneManager->getVideoDriver();

    driver->setMaterial(Material);
    driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);
    driver->drawVertexPrimitiveList(Vertices, NumVertices, IndexList, NumVertices, irr::video::EVT_STANDARD, irr::scene::EPT_POINTS, irr::video::EIT_32BIT);

    // for debug purposes only:
    if (DebugDataVisible)
    {
        irr::video::SMaterial debug_mat;
        debug_mat.Lighting = false;
        debug_mat.AntiAliasing=0;
        driver->setMaterial(debug_mat);
        // show normals
        if (DebugDataVisible & irr::scene::EDS_NORMALS)
        {
            const irr::f32 debugNormalLength = SceneManager->getParameters()->getAttributeAsFloat(irr::scene::DEBUG_NORMAL_LENGTH);
            const irr::video::SColor debugNormalColor = SceneManager->getParameters()->getAttributeAsColor(irr::scene::DEBUG_NORMAL_COLOR);
            // draw normals
            for (irr::u32 i=0; i < NumVertices; ++i)
            {
                irr::core::vector3df normalizedNormal = Vertices[i].Normal;
                normalizedNormal.normalize();
                const irr::core::vector3df& pos = Vertices[i].Pos;
                driver->draw3DLine(pos, pos + (normalizedNormal * debugNormalLength), debugNormalColor);
            }
        }

        debug_mat.ZBuffer = irr::video::ECFN_DISABLED;
        debug_mat.Lighting = false;
        driver->setMaterial(debug_mat);

        if (DebugDataVisible & irr::scene::EDS_BBOX || DebugDataVisible & irr::scene::EDS_BBOX_BUFFERS)
            driver->draw3DBox(Box, irr::video::SColor(255,255,255,255));
    }
}

/*
And finally we create three small additional methods.
irr::scene::ISceneNode::getBoundingBox() returns the bounding box of
this scene node, irr::scene::ISceneNode::getMaterialCount() returns the
amount of materials in this scene node (our tetraeder only has one
material), and irr::scene::ISceneNode::getMaterial() returns the
material at an index. Because we have only one material here, we can
return the only one material, assuming that no one ever calls
getMaterial() with an index greater than 0.
*/
const irr::core::aabbox3d<irr::f32>& PointCloudSceneNode::getBoundingBox() const
{
    return Box;
}

irr::u32 PointCloudSceneNode::getMaterialCount() const
{
    return 1;
}

irr::video::SMaterial& PointCloudSceneNode::getMaterial(irr::u32 /* i */)
{
    return Material;
}

// this is the custom initiation function

void PointCloudSceneNode::initializeVertices(PointCloud *pointCloud, irr::s32 maxPointsToRender)
{
    if (pointCloud) // load up the buffer objects
    {
        irr::u32 stride;
        if (maxPointsToRender >= pointCloud->GetNPoints())
        {
            stride = 1;
            NumVertices = pointCloud->GetNPoints();
        }
        else
        {
            stride = pointCloud->GetNPoints() / maxPointsToRender;
            if (pointCloud->GetNPoints() % maxPointsToRender) stride++;
            NumVertices = pointCloud->GetNPoints() / stride;
        }

        if (Vertices) delete [] Vertices;
        Vertices = new irr::video::S3DVertex[NumVertices];
        if (IndexList) delete [] IndexList;
        IndexList = new irr::u32[NumVertices];

        irr::video::S3DVertex v;
        irr::u32 i, j;
        const Point *pointList = pointCloud->GetPointList();
        for (j = 0; j < NumVertices; j++)
        {
            i = j * stride;
            v.Pos.set(pointList[i].x, pointList[i].y, pointList[i].z);
            v.Color.set(pointList[i].a, pointList[i].r, pointList[i].g, pointList[i].b);
            v.Normal.set(pointList[i].nx, pointList[i].ny, pointList[i].nz);
            // v.TCoords.set(tu, tv);
            Vertices[j] = v;
            IndexList[j] = j;
        }

        Vector3f minBound = pointCloud->GetMinBound();
        Vector3f maxBound = pointCloud->GetMaxBound();
        Box = irr::core::aabbox3df(minBound.x, minBound.y, minBound.z, maxBound.x, maxBound.y, maxBound.z);

    }
}


