#pragma once
#include <vector>

typedef std::vector< std::vector<BYTE> > TImage;

// HGW algo (with maximum)
void MorphOp( BitmapData& data, const size_t filterSize );