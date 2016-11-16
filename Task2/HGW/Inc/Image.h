#pragma once

#include <vector>

typedef std::vector< std::vector< BYTE > > TImage;

class CImage {
public:
	explicit CImage( const BitmapData& bmpData );

	void EasyDilate( TImage& result, const size_t radius ) const;
	void HGWDilate( TImage& result, const size_t radius ) const;
	void GetData( BitmapData& bmpData ) const;

private:
	TImage image;
	size_t height;
	size_t width;

	BYTE getMaxValue( int i, int j, int rad ) const;
	void hgwProcessing( const TImage& image, const size_t filterSize, TImage& result ) const;
	void oneDimensionalHGW( const std::vector<BYTE>& a, const size_t radius, std::vector<BYTE>& result ) const;
};
