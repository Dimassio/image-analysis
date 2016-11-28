#pragma once

#include <vector>

typedef std::vector< std::vector< BYTE > > TImage;

class CImage {
public:
	explicit CImage( const BitmapData& bmpData );
	void GetData( BitmapData& data ) const;
	void NICKBinarization( const size_t windowSize );

private:
	// Серое изображение
	TImage image;
	size_t height;
	size_t width;
};
