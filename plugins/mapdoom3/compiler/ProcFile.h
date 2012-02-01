#pragma once

#include <boost/shared_ptr.hpp>
#include <vector>
#include "ibrush.h"
#include "ipatch.h"
#include "ishaders.h"

#include "math/AABB.h"
#include "render/ArbitraryMeshVertex.h"

#include "PlaneSet.h"
#include "ProcWinding.h"
#include "ProcLight.h"
#include "ProcBrush.h"
#include "BspTree.h"

namespace model { class IModelSurface; }
class IPatch;

namespace map
{

struct HashVertex;
struct OptVertex;

// chains of ProcTri are the general unit of processing
struct ProcTri
{
	MaterialPtr					material;
	const ProcFace*				mergeGroup;		// we want to avoid merging triangles
	const model::IModelSurface* mergeSurf;		// from different fixed groups, like guiSurfs and mirrors
	const IPatch*				mergePatch;
	int							planeNum;		

	ArbitraryMeshVertex			v[3];

	const HashVertex*			hashVert[3];	// for T-junction pass
	OptVertex*					optVert[3];		// for optimization

	ProcTri() :
		mergeGroup(NULL),
		mergeSurf(NULL),
		mergePatch(NULL)
	{}
};
typedef std::vector<ProcTri> ProcTris;

#define MAX_GROUP_LIGHTS 16

struct ProcOptimizeGroup
{
	AABB				bounds;			// set in CarveGroupsByLight

	// all of these must match to add a triangle to the triList
	bool				smoothed;		// curves will never merge with brushes
	std::size_t			planeNum;
	std::size_t			areaNum;
	MaterialPtr			material;
	int					numGroupLights;
	ProcLight			groupLights[MAX_GROUP_LIGHTS];	// lights effecting this list
	const ProcFace*		mergeGroup;			// if this differs (guiSurfs, mirrors, etc), the
	const model::IModelSurface* mergeSurf;	// groups will not be combined into model surfaces
	const IPatch*		mergePatch;			// after optimization
	Vector4				texVec[2];

	bool				surfaceEmitted;

	ProcTris			triList;
	ProcTris			regeneratedTris;	// after each island optimization
	Vector3				axis[2];			// orthogonal to the plane, so optimization can be 2D
};

struct ProcArea
{
	typedef std::vector<ProcOptimizeGroup> OptimizeGroups;
	OptimizeGroups	groups;
};

// A primitive can either be a brush or a patch,
// so only one of the pointers is non-NULL
struct ProcPrimitive
{
	ProcBrushPtr	brush;
	ProcTris		patch;	// this is empty for brushes
};

struct ProcEntity
{
	// The reference into the scenegraph
	IEntityNodePtr	mapEntity;

	Vector3			origin;

	// Each entity has 0..N primitives
	typedef std::vector<ProcPrimitive> Primitives;
	Primitives		primitives;

	BspTree			tree;

	std::size_t		numAreas;

	typedef std::vector<ProcArea> Areas;
	Areas			areas;	// populated in putPrimitiveInAreas()

	ProcEntity(const IEntityNodePtr& entityNode) :
		mapEntity(entityNode),
		numAreas(0)
	{}
};

struct ProcInterAreaPortal
{
	std::size_t area0;
	std::size_t area1;
	ProcFace*	side;
};

class LeakFile;
typedef boost::shared_ptr<LeakFile> LeakFilePtr;

/**
 * This class represents the processed data (entity models and shadow volumes)
 * as generated by the dmap compiler. Use the saveToFile() method to write the
 * data into the .proc file.
 */
class ProcFile
{
public:
	typedef std::vector<ProcEntityPtr> ProcEntities;
	ProcEntities entities;

	// All the planes in the map
	PlaneSet planes;

	std::size_t numPortals;
	std::size_t numPatches;
	std::size_t numWorldBrushes;
	std::size_t numWorldTriSurfs;

	AABB mapBounds;

	typedef std::vector<ProcLight> ProcLights;
	ProcLights lights;

	LeakFilePtr leakFile;

	typedef std::vector<ProcInterAreaPortal> InterAreaPortals;
	InterAreaPortals interAreaPortals;

	ProcFile() :
		numPortals(0),
		numPatches(0),
		numWorldBrushes(0),
		numWorldTriSurfs(0)
	{}

	void saveToFile(const std::string& path)
	{
		// TODO
	}

	bool hasLeak() const
	{
		return leakFile;
	}

	static const char* const Extension()
	{
		return ".proc";
	}
};
typedef boost::shared_ptr<ProcFile> ProcFilePtr;

} // namespace
