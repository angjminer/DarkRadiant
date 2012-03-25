#pragma once

#include "ieclass.h"
#include "irender.h"

#include "math/Vector3.h"
#include "math/AABB.h"
#include "string/string.h"

#include "parser/DefTokeniser.h"

#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

/* FORWARD DECLS */

class Shader;

namespace eclass
{

class Doom3EntityClass;
typedef boost::shared_ptr<Doom3EntityClass> Doom3EntityClassPtr;

/**
 * Implementation of the IEntityClass interface. This represents a single
 * Doom 3 entity class, such as "light_moveable" or "monster_mancubus".
 */
class Doom3EntityClass
: public IEntityClass
{
    typedef boost::shared_ptr<std::string> StringPtr;

    class StringCompareFunctor :
        public std::binary_function<std::string, std::string, bool>
    {
    public:
        bool operator()(const StringPtr& lhs, const StringPtr& rhs) const
        {
            //return boost::algorithm::ilexicographical_compare(lhs, rhs); // this is slow!
            return string_compare_nocase(lhs->c_str(), rhs->c_str()) < 0;
        }
    };

    // The name of this entity class
    std::string _name;

    // Should this entity type be treated as a light?
    bool _isLight;

    // Colour of this entity and flag to indicate it has been specified
    Vector3 _colour;
    bool _colourSpecified;
    bool _colourTransparent;

    // Shader versions of the colour
    std::string _fillShader;
    std::string _wireShader;

    // Does this entity have a fixed size?
    bool _fixedSize;

    // Map of named EntityAttribute structures. EntityAttributes are picked
    // up from the DEF file during parsing. Ignores key case.

    // greebo: I've changed the EntityAttributeMap key type to StringPtr, to save
    // more than 130 MB of string data used for just the keys. A default TDM installation
    // has about 780k entity class attributes after resolving inheritance.
    // However, the memory saving comes with a performance cost since we need 
    // to convert the incoming std::string queries to StringRefs before passing
    // them to std::map::find(). During a regular DarkRadiant startup 
    // there are about 64k find() operations, a single idLight
    // creation costs about 30-40 find() operations, which is ok I guess.

    typedef std::map<StringPtr, EntityClassAttribute, StringCompareFunctor> EntityAttributeMap;
    EntityAttributeMap _attributes;

    // The model and skin for this entity class (if it has one)
    std::string _model;
    std::string _skin;

    // Flag to indicate inheritance resolved. An EntityClass resolves its
    // inheritance by copying all values from the parent onto the child,
    // after recursively instructing the parent to resolve its own inheritance.
    bool _inheritanceResolved;

    // Name of the mod owning this class
    std::string _modName;

    // The empty attribute
    EntityClassAttribute _emptyAttribute;

    // The list of strings containing the ancestors and this eclass itself.
    InheritanceChain _inheritanceChain;

    // Any def_attached entities. Each attachment has an entity class, a
    // position and optionally a name.
    struct Attachment
    {
        std::string className;
        std::string name;
        std::string posName;
    };
    typedef std::map<unsigned, Attachment> Attachments;
    Attachments _attachments;

    // The time this def has been parsed
    std::size_t _parseStamp;

    // Emitted when contents are reloaded
    sigc::signal<void> _changedSignal;

private:
    // Clear all contents (done before parsing from tokens)
    void clear();
    void parseDefAttachKeys(const std::string& key, const std::string& value);
    void parseEditorSpawnarg(const std::string& key, const std::string& value);
    // Rebuilds the inheritance chain (called after inheritance is resolved)
    void buildInheritanceChain();

public:

    /**
     * Static function to create a default entity class.
     *
     * @param name
     * The name of the entity class to create.
     *
     * @param brushes
     * Whether the entity contains brushes or not.
     */
    static Doom3EntityClassPtr create(const std::string& name, bool brushes);

    /**
     * Default constructor.
     *
     * @param name
     * Entity class name.
     *
     * @param colour
     * Display colour for this entity.
     */
    Doom3EntityClass(const std::string& name,
                     const Vector3& colour = Vector3(-1, -1, -1),
                     bool fixedSize = false,
                     const Vector3& mins = Vector3(1, 1, 1),
                     const Vector3& maxs = Vector3(-1, -1, -1));

    /** Destructor.
     */
    ~Doom3EntityClass();

    /** Return the name of this entity class.
     */
    const std::string& getName() const;

    void setIsLight(bool val);

    // IEntityClass implementation
    sigc::signal<void> changedSignal() const;
    bool isFixedSize() const;
    AABB getBounds() const;
    bool isLight() const;
    void setColour(const Vector3& colour);
    const Vector3& getColour() const;
    const std::string& getWireShader() const;
    const std::string& getFillShader() const;
    void addAttribute(const EntityClassAttribute& attribute);
    EntityClassAttribute& getAttribute(const std::string& name);
    const EntityClassAttribute& getAttribute(const std::string& name) const;
    EntityClassAttributeList getAttributeList(const std::string& name) const;
    void forEachClassAttribute(boost::function<void(const EntityClassAttribute&)>,
                               bool) const;

    /** Set a model on this entity class.
     *
     * @param
     * The VFS model path.
     */
    void setModelPath(const std::string& path) {
        _fixedSize = true;
        _model = path;
    }

    /** Return the model path
     */
    const std::string& getModelPath() const {
        return _model;
    }

    /** Set the skin.
     */
    void setSkin(const std::string& skin) {
        _skin = skin;
    }

    /** Get the skin.
     */
    const std::string& getSkin() const {
        return _skin;
    }

    /**
     * Returns the inheritance chain (including this eclass).
     */
    virtual const InheritanceChain& getInheritanceChain();

    /**
     * Resolve inheritance for this class.
     *
     * @param classmap
     * A reference to the global map of entity classes, which should be searched
     * for the parent entity.
     */
    typedef std::map<std::string, Doom3EntityClassPtr> EntityClasses;
    void resolveInheritance(EntityClasses& classmap);

    /**
     * Return the mod name.
     */
    std::string getModName() const {
        return _modName;
    }

    /**
     * Set the mod name.
     */
    void setModName(const std::string& mn) {
        _modName = mn;
    }

    // Initialises this class from the given tokens
    void parseFromTokens(parser::DefTokeniser& tokeniser);

    void setParseStamp(std::size_t parseStamp)
    {
        _parseStamp = parseStamp;
    }

    std::size_t getParseStamp() const
    {
        return _parseStamp;
    }
};

/**
 * Pointer typedef.
 */
typedef boost::shared_ptr<Doom3EntityClass> Doom3EntityClassPtr;

}
