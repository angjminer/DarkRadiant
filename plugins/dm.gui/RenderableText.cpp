#include "RenderableText.h"

#include "itextstream.h"
#include "iregistry.h"

#include "math/matrix.h"
#include <vector>
#include <list>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "TextParts.h"
#include "GuiWindowDef.h"
#include "math/FloatTools.h"

namespace gui
{

	namespace
	{
		const std::string RKEY_SMALLFONT_LIMIT("game/defaults/guiSmallFontLimit");
		const std::string RKEY_MEDIUMFONT_LIMIT("game/defaults/guiMediumFontLimit");
	}

RenderableText::RenderableText(const GuiWindowDef& owner) :
	_owner(owner)
{}

void RenderableText::realiseFontShaders()
{
	_font->getGlyphSet(_resolution)->realiseShaders();
}

void RenderableText::render()
{
	// Add each renderable character batch to the collector
	for (CharBatches::const_iterator i = _charBatches.begin(); i != _charBatches.end(); ++i)
	{
		// Switch to this shader
		glBindTexture(GL_TEXTURE_2D, i->first->getMaterial()->getEditorImage()->getGLTexNum());

		// Submit geometry
		i->second->render();
	}
}

void RenderableText::recompile()
{
	_charBatches.clear();
	
	ensureFont();

	if (_font == NULL) return; // Rendering not possible

	std::string text = _owner.getText();

	typedef std::vector<TextLinePtr> TextLines;
	TextLines lines;

	// Split the text into paragraphs
	std::vector<std::string> paragraphs;
	boost::algorithm::split(paragraphs, text, boost::algorithm::is_any_of("\n"));

	fonts::IGlyphSet& glyphSet = *_font->getGlyphSet(_resolution);

	// Calculate the final scale of the glyphs
	float scale = _owner.textscale * glyphSet.getGlyphScale();

	// Calculate the line height
	// This is based on a series of measurements using the Carleton font.
	double lineHeight = lrint(_owner.textscale * 51 + 5);

	// The distance from the top of the rectangle to the baseline
	double startingBaseLine = lrint(_owner.textscale * 51 + 2);

	for (std::size_t p = 0; p < paragraphs.size(); ++p)
	{
		// Split the paragraphs into words
		std::list<std::string> words;
		boost::algorithm::split(words, paragraphs[p], boost::algorithm::is_any_of(" \t"));

		// Add the words to lines
		TextLinePtr curLine(new TextLine(_owner.rect[2], scale));

		while (!words.empty())
		{
			// Empty words are stemming from an extra space character, re-add that
			if (words.front().empty()) 
			{
				curLine->addChar(' ', glyphSet, true);
				words.pop_front();
				continue;
			}

			bool added = curLine->addWord(words.front(), glyphSet);

			if (added)
			{
				words.pop_front();

				if (!words.empty())
				{
					// Add a space after each word (noclipped) if more words are following
					curLine->addChar(' ', glyphSet, true);
				}
				
				continue;
			}

			// Not added
			if (curLine->empty())
			{
				// Line empty, but still not fitting, do it character-wise
				std::string word = words.front();
				words.pop_front();

				while (!word.empty())
				{
					// Take the first character
					if (curLine->addChar(word[0], glyphSet))
					{
						// Character added, remove from string
						word.erase(0, 1);
						continue;
					}
					else
					{
						// Not enough space, add one more character then break
						curLine->addChar(word[0], glyphSet, true);
						word.erase(0, 1);
						break;
					}
				}

				// add the rest of the word to the front of the queue
				if (!word.empty())
				{
					words.push_front(word);
				}
			}
			
			// Trim any extra space from the end of the line
			curLine->removeTrailingSpace();
			
			// Line finished, consider alignment and vertical offset
			curLine->offset(Vector2(
				getAlignmentCorrection(curLine->getWidth()), // horizontal correction
				lineHeight * lines.size() + startingBaseLine // vertical correction
			));

			lines.push_back(curLine);

			// Allocate a new line, and proceed
			curLine = TextLinePtr(new TextLine(_owner.rect[2], scale));
		}

		// Trim any extra space from the end of the line
		curLine->removeTrailingSpace();

		// Add that line we started, even if it's an empty one
		curLine->offset(
			Vector2(
				getAlignmentCorrection(curLine->getWidth()), 
				lineHeight * lines.size() +  + startingBaseLine
			)
		);
		lines.push_back(curLine);
	}

	// Now sort the aligned characters into separate renderables, one per shader
	for (TextLines::const_iterator line = lines.begin(); line != lines.end(); ++line)
	{
		// Move the lines into our GUI rectangle
		(*line)->offset(Vector2(_owner.rect[0], _owner.rect[1]));

		for (TextLine::Chars::const_iterator c = (*line)->getChars().begin();
			 c != (*line)->getChars().end(); ++c)
		{
			CharBatches::iterator batch = _charBatches.find(c->glyph->shader);

			if (batch == _charBatches.end())
			{
				RenderableCharacterBatchPtr b(new RenderableCharacterBatch);

				std::pair<CharBatches::iterator, bool> result = _charBatches.insert(
					CharBatches::value_type(c->glyph->shader, b)
				);

				batch = result.first;
			}

			batch->second->addGlyph(*c);
		}
	}

	// Compile the vertex buffer objects
	for (CharBatches::iterator i = _charBatches.begin(); i != _charBatches.end(); ++i)
	{
		i->second->compile();
	}
}

double RenderableText::getAlignmentCorrection(double lineWidth)
{
	double xoffset = 0;

	switch (_owner.textalign)
	{
	case 0: // left
		// Somehow D3 adds a 2 pixel offset to the left
		xoffset = 2; 
		break;
	case 1: // center
		// Somehow D3 adds a 1 pixel offset to the left
		xoffset = 1 + (_owner.rect[2] - lineWidth) / 2;
		break;
	case 2: // right
		xoffset = _owner.rect[2] - lineWidth;
		break;
	};

	return xoffset;
}

void RenderableText::ensureFont()
{
	if (_font != NULL) return; // already realised

	_font = GlobalFontManager().findFontInfo(_owner.font);

	if (_font == NULL)
	{
		globalWarningStream() << "Cannot find font " << _owner.font 
			<< " in windowDef " << _owner.name << std::endl;
		return;
	}

	// Determine resolution
	if (_owner.textscale <= GlobalRegistry().getFloat(RKEY_SMALLFONT_LIMIT))
	{
		_resolution = fonts::Resolution12;
	}
	else if (_owner.textscale <= GlobalRegistry().getFloat(RKEY_MEDIUMFONT_LIMIT))
	{
		_resolution = fonts::Resolution24;
	}
	else 
	{
		_resolution = fonts::Resolution48;
	}

	// Ensure that the font shaders are realised
	realiseFontShaders();
}

} // namespace
