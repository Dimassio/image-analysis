#pragma once

#include <vector>

typedef std::vector< std::vector< BYTE > > TImage;

enum TMorphOp {
	MO_Dilate,
	MO_Erase
};

class CImage {
public:
	explicit CImage( const BitmapData& bmpData, TMorphOp );

	// Не перезаписываем исходное изображение, 
	// чтобы можно было несколько раз экспериментировать
	void EasyMorph( TImage& result, const size_t radius ) const;
	void HGWMorph( TImage& result, const size_t radius ) const;

private:
	TMorphOp currOp;
	TImage image;
	size_t height;
	size_t width;

	BYTE getMorphValue( int i, int j, int rad ) const;
	void hgwProcessing( const TImage& image, const size_t filterSize, TImage& result ) const;
	void oneDimensionalHGW( const std::vector<BYTE>& input, const size_t radius, std::vector<BYTE>& result ) const;
	BYTE calculateMorphValue( BYTE a, BYTE b ) const;
};
