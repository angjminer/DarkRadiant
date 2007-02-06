#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

#include "iimage.h"
#include "ishaders.h"
#include "iregistry.h"
#include "preferencesystem.h"
#include "itextures.h"
typedef unsigned char byte;

namespace shaders {

class TextureManipulator :
 	public RegistryKeyObserver,
 	public PreferenceConstructor
{
	// The gamma correction table
	byte _gammaTable[256];
	
	// The currently active gamma value (0.0...1.0)
	float _gamma;
	
	// The filtering mode (mipmap_linear and such)
	ETexturesMode _textureMode;
	
public:
	TextureManipulator();
	
	// RegistryKeyObserver implementation
	void keyChanged();

	// PreferenceConstructor implementation to construct the prefpage
	void constructPreferencePage(PreferenceGroup& group);
	
	void resampleTexture(const void *indata, int inwidth, int inheight, 
						 void *outdata, int outwidth, int outheight, int bytesperpixel);
	
	void mipReduce(byte *in, byte *out, 
				   int width, int height, 
				   int destwidth, int destheight);

	/* greebo: Returns the readily fabricated pixel data, that passed
	 * a bunch of stages (gamma calculation, mip reduction, stretching) 
	 */
	Image* getProcessedImage(Image* input);

	/* greebo: Performs a fast scan over the pixel data, taking every
	 * 20th pixel to determine the representative flat shade colour 
	 */
	Colour3 getFlatshadeColour(Image* input);

private:

	// Returns the gamma corrected image taken from <input>
	// (Does not allocate new memory)
	Image* processGamma(Image* input);
	
	// Recalculates the gamma table according to the given gamma value
	// This is called on first startup or if the user changes the value
	void calculateGammaTable();
	
	/* greebo: Sends the openGL texturemode commands according to the internal
	 * texture mode member variable. (e.g. MIPMAP_LINEAR)
	 */
	void setTextureParameters();
	
	// Converts the <int> to the ETexturesMode enumeration
	ETexturesMode readTextureMode(const unsigned int& mode);
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

} // namespace shaders

#endif /*TEXTUREMANIPULATOR_H_*/
