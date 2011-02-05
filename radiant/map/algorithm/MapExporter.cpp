#include "MapExporter.h"

#include <ostream>
#include "i18n.h"
#include "itextstream.h"
#include "iregistry.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ientity.h"
#include "igroupnode.h"
#include "imainframe.h"

#include "scenelib.h"
#include "string/string.h"

namespace map
{

	namespace
	{
		const char* const RKEY_FLOAT_PRECISION = "/mapFormat/floatPrecision";
		const char* const RKEY_MAP_SAVE_STATUS_INTERLEAVE = "user/ui/map/saveStatusInterleave";
	}

MapExporter::MapExporter(IMapWriter& writer, const scene::INodePtr& root, std::ostream& mapStream, std::size_t nodeCount) :
	_writer(writer),
	_mapStream(mapStream),
	_root(root),
	_dialogEventLimiter(GlobalRegistry().getInt(RKEY_MAP_SAVE_STATUS_INTERLEAVE)),
	_totalNodeCount(nodeCount),
	_curNodeCount(0)
{
	if (_totalNodeCount > 0)
	{
		enableProgressDialog();
	}

	// Prepare the output stream
	game::IGamePtr curGame = GlobalGameManager().currentGame();
	assert(curGame != NULL);

	xml::NodeList nodes = curGame->getLocalXPath(RKEY_FLOAT_PRECISION);
	assert(!nodes.empty());

	int precision = strToInt(nodes[0].getAttributeValue("value"));
	_mapStream.precision(precision);

	prepareScene();

	// Add origin to func_* children before writing
	try
	{
		_writer.beginWriteMap(_mapStream);
	}
	catch (IMapWriter::FailureException& ex)
	{
		globalErrorStream() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}
}

MapExporter::~MapExporter()
{
	try
	{
		_writer.endWriteMap(_mapStream);
	}
	catch (IMapWriter::FailureException& ex)
	{
		globalErrorStream() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}

	finishScene();
}

void MapExporter::enableProgressDialog()
{
	_dialog = gtkutil::ModalProgressDialogPtr(
		new gtkutil::ModalProgressDialog(
			GlobalMainFrame().getTopLevelWindow(), _("Writing map")
		)
	);
}

void MapExporter::disableProgressDialog()
{
	_dialog.reset();
}

bool MapExporter::pre(const scene::INodePtr& node)
{
	try
	{
		Entity* entity = Node_getEntity(node);

		if (entity != NULL)
		{
			// Progress dialog handling
			onNodeProgress();
			
			_writer.beginWriteEntity(*entity, _mapStream);

			return true;
		}

		IBrush* brush = Node_getIBrush(node);

		if (brush != NULL)
		{
			// Progress dialog handling
			onNodeProgress();

			_writer.beginWriteBrush(*brush, _mapStream);
			return true;
		}

		IPatch* patch = Node_getIPatch(node);

		if (patch != NULL)
		{
			// Progress dialog handling
			onNodeProgress();

			_writer.beginWritePatch(*patch, _mapStream);
			return true;
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		globalErrorStream() << "Failure exporting a node (pre): " << ex.what() << std::endl;
	}

	return true; // full traversal
}

void MapExporter::post(const scene::INodePtr& node)
{
	try
	{
		Entity* entity = Node_getEntity(node);

		if (entity != NULL)
		{
			_writer.endWriteEntity(*entity, _mapStream);
			return;
		}

		IBrush* brush = Node_getIBrush(node);

		if (brush != NULL)
		{
			_writer.endWriteBrush(*brush, _mapStream);
			return;
		}

		IPatch* patch = Node_getIPatch(node);

		if (patch != NULL)
		{
			_writer.endWritePatch(*patch, _mapStream);
			return;
		}
	}
	catch (IMapWriter::FailureException& ex)
	{
		globalErrorStream() << "Failure exporting a node (post): " << ex.what() << std::endl;
	}
}

void MapExporter::onNodeProgress()
{
	_curNodeCount++;

	// Update the dialog text. This will throw an exception if the cancel
	// button is clicked, which we must catch and handle.
	if (_dialog && _dialogEventLimiter.readyForEvent())
	{
		std::string text = (boost::format(_("Writing node %d")) % _curNodeCount).str();
		_dialog->setTextAndFraction(
			text, 
			static_cast<double>(_curNodeCount) / static_cast<double>(_totalNodeCount)
		);
	}
}

void MapExporter::prepareScene()
{
	// Disable texture lock during this process
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	// Local helper to remove the origins
	class OriginRemover :
		public scene::NodeVisitor
	{
	public:
		bool pre(const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			// Check for an entity
			if (entity != NULL)
			{
				// greebo: Check for a Doom3Group
				scene::GroupNodePtr groupNode = Node_getGroupNode(node);

				// Don't handle the worldspawn children, they're safe&sound
				if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn")
				{
					groupNode->removeOriginFromChildren();
					// Don't traverse the children
					return false;
				}
			}

			return true;
		}
	} remover;

	Node_traverseSubgraph(_root, remover);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

void MapExporter::finishScene()
{
	// Disable texture lock during this process
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	// Local helper to add origins
	class OriginAdder :
		public scene::NodeVisitor
	{
	public:
		// NodeVisitor implementation
		bool pre(const scene::INodePtr& node)
		{
			Entity* entity = Node_getEntity(node);

			// Check for an entity
			if (entity != NULL)
			{
				// greebo: Check for a Doom3Group
				scene::GroupNodePtr groupNode = Node_getGroupNode(node);

				// Don't handle the worldspawn children, they're safe&sound
				if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn")
				{
					groupNode->addOriginToChildren();
					// Don't traverse the children
					return false;
				}
			}
			return true;
		}
	} adder;

	Node_traverseSubgraph(_root, adder);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

} // namespace