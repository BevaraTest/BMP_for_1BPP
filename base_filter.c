#include <gpac/filters.h>



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* Type definitions */
#ifndef UINT // 4 bytes
	#define UINT	unsigned long int
#endif

#ifndef USHORT //2 bytes
	#define USHORT	unsigned short
#endif

#ifndef SHORT 
	#define SHORT	short
#endif

#ifndef UCHAR // 1 byte
	#define UCHAR	unsigned char
#endif


typedef enum
{
	BMP_OK = 0,				/* No error */
	BMP_ERROR,				/* General error */
	BMP_OUT_OF_MEMORY,		/* Could not allocate enough memory to complete the operation */
	BMP_IO_ERROR,			/* General input/output error */
	BMP_FILE_NOT_FOUND,		/* File not found */
	BMP_FILE_NOT_SUPPORTED,	/* File is not a supported BMP variant */
	BMP_FILE_INVALID,		/* File is not a BMP image or is an invalid BMP */
	BMP_INVALID_ARGUMENT,	/* An argument is invalid or out of range */
	BMP_TYPE_MISMATCH,		/* The requested action is not compatible with the BMP's type */
	BMP_ERROR_NUM
} BMP_STATUS;


typedef  enum
 {
   BI_RGB = 0x0000,
   BI_RLE8 = 0x0001,
   BI_RLE4 = 0x0002,
   BI_BITFIELDS = 0x0003
//    ,  //TAKEN FROM WINDOWS
//    BI_JPEG = 0x0004,
//    BI_PNG = 0x0005,
//    BI_CMYK = 0x000B,
//    BI_CMYKRLE8 = 0x000C,
//    BI_CMYKRLE4 = 0x000D
 } Compression;

/* Mask structure*/
struct Bitfield_Mask
{
	UINT mask;
	UINT lowBit;
	UINT range;

};

/* Bitmap header */
struct BMP_Header
{
	USHORT		Magic;				/* Magic identifier: "BM" */
	UINT		FileSize;			/* Size of the BMP file in bytes */
	USHORT		Reserved1;			/* Reserved */
	USHORT		Reserved2;			/* Reserved */
	UINT		DataOffset;			/* Offset of image data relative to the file's start */
	UINT		HeaderSize;			/* Size of the header in bytes */
	UINT		Width;				/* Bitmap's width */
	UINT		Height;				/* Bitmap's height */
	USHORT		Planes;				/* Number of color planes in the bitmap */
	USHORT		BitsPerPixel;		/* Number of bits per pixel */
	UINT		CompressionType;	/* Compression type */
	UINT		ImageDataSize;		/* Size of uncompressed image's data */
	UINT		HPixelsPerMeter;	/* Horizontal resolution (pixels per meter) */
	UINT		VPixelsPerMeter;	/* Vertical resolution (pixels per meter) */
	UINT		ColorsUsed;			/* Number of color indexes in the color table that are actually used by the bitmap */
	UINT		ColorsRequired;		/* Number of color indexes that are required for displaying the bitmap */
	USHORT      Orientation;        /* This is a rarely-used flag that indicates top-down (origin in upper-left, negative Height value => 1)
										vs bottom-up (origin in lower-left, positive Height value => 0)*/
	USHORT      PaletteElementSize;	/* There are 3- or 4-byte palette element sizes*/
	USHORT      PaletteSize;		/* This is the total PaletteSize */
	USHORT		BitMask;			/* Flag to indicate that the WinNtBitFieldMasks should be used instead of the Palette */
	struct 	Bitfield_Mask	RedMask;			/* ask identifying bits of red component with location of high and low bits*/
	struct 	Bitfield_Mask	GreenMask;			/* ask identifying bits of gree component with location of high and low bits*/
	struct 	Bitfield_Mask	BlueMask;			/* ask identifying bits of blue component with location of high and low bits*/
};


/* Private data structure */
struct BMP_struct
{
	struct BMP_Header	Header;
	UCHAR*		Palette;
	UCHAR*		Data;
	
};

/* Meta info */
int			BMP_GetWidth( );
int			BMP_GetHeight( );



/* Holds the last error code */
static BMP_STATUS BMP_LAST_ERROR_CODE;



/*********************************** Forward declarations **********************************/
int		ReadHeader	( const char* bmp_data, const int size );
int		ReadUINT	(  UINT* x, const char* bmp_data,  const int size );
int		ReadINT	(  int* x, const char* bmp_data,  const int size );
int		ReadUSHORT	(  USHORT *x, const char* bmp_data, const int size );
int		BitfieldRange(UINT mask, UINT *range, UINT *lowBit);
int 	dec1(const char* bmp_data);

/* BMP representation and helper variables*/
struct BMP_struct * bmp;
long dataInd;
int scanLinePadding;


/**************************************************************
	Returns the image's width.
**************************************************************/
int BMP_GetWidth(  )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return -1;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return ( bmp->Header.Width );
}


/**************************************************************
	Returns the image's height.
**************************************************************/
int BMP_GetHeight( )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return -1;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return ( bmp->Header.Height );
}

/*********************************** Private methods **********************************/


/**************************************************************
	Reads the BMP file and DIB headers into the data structure.
	Returns BMP_OK on success.
**************************************************************/
int	ReadHeader(const char* bmp_data, const int size )
{
	if ( bmp == NULL  )
	{
		return BMP_INVALID_ARGUMENT;
	}

	/* The header's fields are read one by one, and converted from the format's
	little endian to the system's native representation. */

	/* the first part is the general file header */
	if ( !ReadUSHORT( &( bmp->Header.Magic ),bmp_data,size ) )			return BMP_IO_ERROR;

	/*early sanity check*/
	
	/* check the magic number options: BM, BA, CI, CP, IC, PT (little endian)*/
	if (  bmp->Header.Magic != 0x4D42 && bmp->Header.Magic != 0x4D41 && bmp->Header.Magic != 0x4943 && bmp->Header.Magic != 0x5043 && bmp->Header.Magic != 0x5450)
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		free( bmp );
		return BMP_LAST_ERROR_CODE;
	}

	/* let's init this thing in case something squeaks by us - just do field-by-field*/
	bmp->Header.FileSize = 0;
	bmp->Header.Reserved1 = 0;
	bmp->Header.Reserved2 = 0;
	bmp->Header.DataOffset = 0;
	bmp->Header.HeaderSize = 0;
	bmp->Header.Width = 0;
	bmp->Header.Height = 0;
	bmp->Header.Planes = 0;
	bmp->Header.BitsPerPixel = 0;
	bmp->Header.CompressionType = 0;
	bmp->Header.ImageDataSize = 0;
	bmp->Header.HPixelsPerMeter = 0;
	bmp->Header.VPixelsPerMeter = 0;
	bmp->Header.ColorsUsed = 0;
	bmp->Header.ColorsRequired = 0;
	bmp->Header.Orientation = 0; /* indicates bottom-up bitmap, corresponds to positive Height value*/
	bmp->Header.PaletteElementSize = 0; /* there are 3-byte or 4-byte palettes */
	bmp->Header.PaletteSize = 0;
	bmp->Header.BitMask = 0;
	bmp->Header.RedMask.mask = 0;
	bmp->Header.RedMask.range = 255;
	bmp->Header.RedMask.lowBit = 0;
	bmp->Header.GreenMask.mask = 0;
	bmp->Header.GreenMask.range = 255;
	bmp->Header.GreenMask.lowBit = 0;
	bmp->Header.BlueMask.mask = 0;
	bmp->Header.BlueMask.range = 255;
	bmp->Header.BlueMask.lowBit = 0;

	/* continue with general header */
	if ( !ReadUINT( &( bmp->Header.FileSize ),bmp_data,size ) )			return BMP_IO_ERROR;
	if ( !ReadUSHORT( &( bmp->Header.Reserved1 ), bmp_data ,size) )		return BMP_IO_ERROR;
	if ( !ReadUSHORT( &( bmp->Header.Reserved2 ), bmp_data,size ) )		return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.DataOffset ), bmp_data,size ) )		return BMP_IO_ERROR;

	/* the second part is the bitmap header --- there are multiple versions of this */
	if ( !ReadUINT( &( bmp->Header.HeaderSize ), bmp_data ,size) )		return BMP_IO_ERROR;

	/* sanity check first */
	if (  bmp->Header.HeaderSize != 12 &&  bmp->Header.HeaderSize != 40 
	 	// TODO: only BMP versions 2 and 3 are supported at this point - the remainder of the formats are TBA
		// && bmp->Header.HeaderSize != 16 
		// && bmp->Header.HeaderSize != 52 // NOT CURRENTLY SUPPORTING THE ADOBE V2 HEADER
		// && bmp->Header.HeaderSize != 56 && bmp->Header.HeaderSize != 64  
		// && bmp->Header.HeaderSize != 108 && bmp->Header.HeaderSize != 124 )
	)
	{
		return BMP_FILE_INVALID;
	}

	
	if (  bmp->Header.HeaderSize == 12)  // Win version 2
	{
		/* The 12-byte header will have just USHORT or SHORT fields */
		USHORT tmp;
		if ( !ReadUSHORT( &tmp, bmp_data,size ) )		return BMP_IO_ERROR;
		bmp->Header.Width = (UINT) (tmp & 0x7FFF);
		if ( !ReadUSHORT( &tmp, bmp_data ,size) )		return BMP_IO_ERROR;
		if ((tmp & 0x8000)) bmp->Header.Orientation = 1; // MSB should indicate bitmap order
		bmp->Header.Height = (UINT) (tmp & 0x7FFF) ;
		if ( !ReadUSHORT( &( bmp->Header.Planes ), bmp_data,size ) )		return BMP_IO_ERROR;
		if ( !ReadUSHORT( &( bmp->Header.BitsPerPixel ), bmp_data ,size) )	return BMP_IO_ERROR;
		if ((bmp->Header.Planes !=1 ) || (bmp->Header.BitsPerPixel == 16) || (bmp->Header.BitsPerPixel == 32))
		{
			return BMP_FILE_INVALID;
		}
		// sanity check the palette
		bmp->Header.PaletteElementSize = 3; /* this format uses a 3-byte palette*/
		bmp->Header.PaletteSize = (bmp->Header.DataOffset - 26); 
		if ( (bmp->Header.PaletteSize)/(pow(2,(bmp->Header.BitsPerPixel)))  != bmp->Header.PaletteElementSize) return BMP_FILE_INVALID;
		// Calculate since not explicitly included
		bmp->Header.ImageDataSize = bmp->Header.FileSize - bmp->Header.DataOffset;
		return BMP_OK;
	}

	/* Non-12-byte header cases*/
	/* Common fields*/
	if ( !ReadUINT( &( bmp->Header.Width ), bmp_data,size ) )			return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.Height ), bmp_data ,size) )			return BMP_IO_ERROR;
	if ( !ReadUSHORT( &( bmp->Header.Planes ), bmp_data,size ) )		return BMP_IO_ERROR;
	if ( !ReadUSHORT( &( bmp->Header.BitsPerPixel ), bmp_data ,size) )	return BMP_IO_ERROR;
	// Extract the orientation
	if ((int)bmp->Header.Height < 0 ) bmp->Header.Orientation = 1; // MSB should indicate bitmap order
		 bmp->Header.Height = (abs((int)bmp->Header.Height)) ;

	/* read the other fields of the BITMAPINFOHEADER*/
	if ( !ReadUINT( &( bmp->Header.CompressionType ), bmp_data ,size) )	return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.ImageDataSize ), bmp_data ,size) )	return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.HPixelsPerMeter ), bmp_data ,size) )	return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.VPixelsPerMeter ), bmp_data,size ) )	return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.ColorsUsed ), bmp_data,size ) )		return BMP_IO_ERROR;
	if ( !ReadUINT( &( bmp->Header.ColorsRequired ), bmp_data ,size) )	return BMP_IO_ERROR;

	if (  bmp->Header.HeaderSize == 40) // Win Version 3s -  sanity check  and calculate the palette
	{
			// check the BBP and compression type for Win 3.x 
			if ((bmp->Header.CompressionType < 0  ) || (bmp->Header.CompressionType > 3  ))
			{
				return BMP_FILE_INVALID;
			}
			if ((bmp->Header.BitsPerPixel != 1  ) && (bmp->Header.BitsPerPixel !=  4  ) &&  (bmp->Header.BitsPerPixel !=  8  ) && (bmp->Header.BitsPerPixel !=  24  )
			 &&  (bmp->Header.BitsPerPixel !=  16  ) && (bmp->Header.BitsPerPixel !=  32  ))
			{
				return BMP_FILE_INVALID;
			}

			// check whether a bit mask is used or calculate the palette
			if ((bmp->Header.CompressionType == 3) && ( bmp->Header.BitsPerPixel == 16 || bmp->Header.BitsPerPixel == 32))
				{
				bmp->Header.BitMask = 1;
				
				// handle the BMP Version 3 bitfieldMask
				// read each mask into an int and calculate its lower bit location and dynamic range
				ReadUINT( &(bmp->Header.RedMask.mask), bmp_data ,size);	
				BitfieldRange(bmp->Header.RedMask.mask, &(bmp->Header.RedMask.range), &(bmp->Header.RedMask.lowBit)); 
				ReadUINT( &(bmp->Header.GreenMask.mask), bmp_data ,size);
				BitfieldRange(bmp->Header.GreenMask.mask, &(bmp->Header.GreenMask.range), &(bmp->Header.GreenMask.lowBit));
				ReadUINT( &( bmp->Header.BlueMask.mask), bmp_data ,size);
				BitfieldRange(bmp->Header.BlueMask.mask, &(bmp->Header.BlueMask.range), &(bmp->Header.BlueMask.lowBit));
				}
			else{
				// calculate the palette
				bmp->Header.PaletteElementSize = 4; /* The Win 3.x format uses a 4-byte palette*/
				bmp->Header.PaletteSize = (bmp->Header.DataOffset - 54); // 14-byte header and 40-byte bitmap header
				// Sanity check if needed - ImageDataSize can be 0 for uncompressed images
				if ((bmp->Header.CompressionType != 0) && (bmp->Header.ImageDataSize != bmp->Header.FileSize - bmp->Header.DataOffset))
					{
						return BMP_FILE_INVALID;
					}
				// Otherwise allocate and read palette (color table), if present 
				if (( bmp->Header.BitsPerPixel <= 8 ) && (bmp->Header.PaletteSize > 0) && (bmp->Header.BitMask == 0))
				{
					
					bmp->Palette = (UCHAR*) malloc( bmp->Header.PaletteSize * sizeof( UCHAR ) );
					if ( bmp->Palette == NULL )
					{
						BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
						free( bmp );
						return BMP_LAST_ERROR_CODE;
					}

					if ( dataInd+ bmp->Header.PaletteSize > size )
					{
						BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
						free( bmp->Palette );
						free( bmp );
						return BMP_LAST_ERROR_CODE;
					}
					else
					{
						memcpy(bmp->Palette,bmp_data+dataInd, bmp->Header.PaletteSize);
						dataInd += bmp->Header.PaletteSize;
					}
				}
				else	/* Not an indexed image */
				{
					bmp->Palette = NULL;
				}	
			}	
	} // end of Win v3.x header read and check  

	return BMP_OK;
}


/**************************************************************
	Reads a little-endian unsigned int from the file.
	Returns non-zero on success.
**************************************************************/
int	ReadINT( int* x, const char* bmp_data,  const int size)
{
	UCHAR little[ 4 ];	/* BMPs use 32 bit ints */

	if ( x == NULL || (dataInd + 4) > size )
	{
		return 0;
	}

	memcpy(&little[0],bmp_data+dataInd,4);
	dataInd = dataInd+4;

	*x = (int) ( little[ 3 ] << 24 | little[ 2 ] << 16 | little[ 1 ] << 8 | little[ 0 ] );

	return 1;
}

/**************************************************************
	Reads a little-endian unsigned int from the file.
	Returns non-zero on success.
**************************************************************/
int	ReadUINT( UINT* x, const char* bmp_data,  const int size)
{
	UCHAR little[ 4 ];	/* BMPs use 32 bit ints */

	if ( x == NULL || (dataInd + 4) > size )
	{
		return 0;
	}

	memcpy(&little[0],bmp_data+dataInd,4);
	dataInd = dataInd+4;

	*x = ( little[ 3 ] << 24 | little[ 2 ] << 16 | little[ 1 ] << 8 | little[ 0 ] );

	return 1;
}


/**************************************************************
	Reads a little-endian unsigned short int from the file.
	Returns non-zero on success.
**************************************************************/
int	ReadUSHORT( USHORT *x, const char* bmp_data, const int size )
{
	UCHAR little[ 2 ];	/* BMPs use 16 bit shorts */

	if ( x == NULL || (dataInd + 2) > size )
	{
		return 0;
	}

	memcpy(&little[0],bmp_data+dataInd,2);
	dataInd = dataInd+2;


	*x = ( little[ 1 ] << 8 | little[ 0 ] );

	return 1;
}


int 	dec1(const char* bmp_data)
{

	int i,j,k;
				scanLinePadding = 0;
				if (bmp->Header.CompressionType == 0) // calculate only if uncompressed
					{						
						scanLinePadding = ((bmp->Header.FileSize - bmp->Header.DataOffset)/bmp->Header.Height)*8 - bmp->Header.Width;
					}

				if (bmp->Header.PaletteSize > 0)
					{
					
					UCHAR *tmp = bmp->Data;
					UINT dataOffset = 0;
					UCHAR paletteIndex = 0; 
					UINT paletteInt = 0;

					k=0; // k will index bits 0=high, 7=low
					for (i=0; i<bmp->Header.Height; ++i)
						{
							for (j=0; j<bmp->Header.Width; ++j) // we'll be grabbing a char at a time from the data
								{
									if (k==0)
									{

										paletteIndex =  *(bmp_data+dataInd + dataOffset); // grab a char
										++dataOffset;
									}
									
									// pull a bit from the char
											paletteInt=  ((paletteIndex & (1 << (7-k))) > 0) ? 1 : 0;

										
											*(tmp) = *(bmp->Palette+paletteInt*bmp->Header.PaletteElementSize+2); 
											++tmp;  
											*(tmp) = *(bmp->Palette+paletteInt*bmp->Header.PaletteElementSize+1); 
											++tmp; 
											*(tmp) = *(bmp->Palette+paletteInt*bmp->Header.PaletteElementSize); 
											++tmp; 
											*(tmp) = 255; ++tmp;
									
									k = (k+1)%8;
								}
					 		// skip the rest of the padding, scanLinePadding is in nibbles
							if (scanLinePadding % 2 == 0)
								{
									dataOffset += (int) (scanLinePadding/8);
								}
							else
								{
									dataOffset += (int) (scanLinePadding -1)/8;
								}
							k=0; // TODO: This is to be confirmed - is there ever the case of non-byte scanline?
				 		}
					
					
					
					}
				else {  // there might be a non-palette case - we do not have a device foreground/background so do black on white
					UCHAR *tmp = bmp->Data;
					UINT dataOffset = 0;
					int bitIdx;
					for (i=0; i<bmp->Header.Height; ++i)
						{
							for (j=0; j<bmp->Header.Width; j+=8) // we'll be grabbing a char at a time from the data
								{
									bitIdx =  *(bmp_data+dataInd + dataOffset); // grab a char
									/* broken out so we can step through colormap*/
									for (k=0; k<8; ++k)
										{ //extract each bit from the char
											if ((bitIdx & (1 << (7-k))) > 0)
											{
												*(tmp) = 255; ++tmp;
												*(tmp) = 255; ++tmp;
												*(tmp) = 255; ++tmp;

											} 
											else
												{
												*(tmp) = 0; ++tmp;
												*(tmp) = 0; ++tmp;
												*(tmp) = 0; ++tmp;												
												}
											*(tmp) = 255; ++tmp;
										}
									dataOffset +=1;
								}
					 		dataOffset += scanLinePadding;
				 		}

				}
		


	return BMP_OK;
}


int		BitfieldRange(UINT mask, UINT *range, UINT *lowBit)
{
	UINT i;
	*lowBit = 0;  // re-inited out of an abundancy of caution
	*range = 255;
	for (i=0; i<32; ++i)
	{
		if (mask & (1<<i))
			{
				*lowBit = i;
				break;
			}
	}
	for (i=31; i>=0; --i)
	{
		if (mask & (1 << i))
			{
				*range = ((1<<(i- *lowBit + 1)) -1 );
				break;
			}
	}


	return 1;
}


typedef struct
{
	u32 opt1;
	Bool opt2;

	GF_FilterPid *src_pid;
	GF_FilterPid *dst_pid;
} GF_BaseFilter;

static void base_filter_finalize(GF_Filter *filter)
{
	//peform any finalyze routine needed, including potential free in the filter context
	//if not needed, set the filter_finalize to NULL


	// inserting test lines
	return GF_OK;

}
static GF_Err BMP1BPP_filter_process(GF_Filter *filter)
{
	u8 *data_dst;
	const u8 *data_src;
	u32 size;

	GF_FilterPacket *pck_dst;
	GF_BaseFilter *stack = (GF_BaseFilter *) gf_filter_get_udta(filter);

	GF_FilterPacket *pck = gf_filter_pid_get_packet(stack->src_pid);
	if (!pck) return GF_OK;
	data_src = gf_filter_pck_get_data(pck, &size);

	
	char * bmp_data = data_src;
	int i;
	dataInd = 0; // init our index into the data 
	scanLinePadding = 0; 

  	BMP_LAST_ERROR_CODE=BMP_OK;

	/* Allocate */
  	bmp = (struct BMP_struct*) malloc( sizeof( struct BMP_struct ) );
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		return BMP_LAST_ERROR_CODE;
	}


	/* Read header */
	if ( ReadHeader( bmp_data, size) != BMP_OK  )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		free( bmp );
		return BMP_LAST_ERROR_CODE;
	}

	/* Check that the bitmap variant is supported */
	/* first check permitted BPP */
	if (  bmp->Header.BitsPerPixel != 32 && bmp->Header.BitsPerPixel != 24 && bmp->Header.BitsPerPixel != 16 && bmp->Header.BitsPerPixel != 8 
		&& bmp->Header.BitsPerPixel != 4 && bmp->Header.BitsPerPixel != 2 && bmp->Header.BitsPerPixel != 1  )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_SUPPORTED;
		free( bmp );
		return BMP_LAST_ERROR_CODE;
	}
	/* next check CompressionType and permitted header sizes */
	if (  bmp->Header.CompressionType != 0 && bmp->Header.HeaderSize != 40 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_SUPPORTED;
		free( bmp );
		return BMP_LAST_ERROR_CODE;
	}

	/* Allocate memory for image data */
	bmp->Data = (UCHAR*) malloc( bmp->Header.Width* bmp->Header.Height * 4); /* forcing RGBA output*/
	if ( bmp->Data == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		free( bmp->Palette );
		free( bmp );
		return BMP_LAST_ERROR_CODE;
	}


	/* Sanity check before decoding*/
	if (dataInd + bmp->Header.ImageDataSize > size )
		{
			BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
			free( bmp->Data );
			free( bmp->Palette );
			free( bmp );
			return BMP_LAST_ERROR_CODE;
		}
	
	/* do the decode */
	if (bmp->Header.BitsPerPixel == 1)
	{
			if (dec1(bmp_data) != BMP_OK) return BMP_FILE_NOT_SUPPORTED;
	}
	else // shouldn't reach here if did earlier sanity check 
			return BMP_FILE_NOT_SUPPORTED;
	
	// Flip, if needed. Leave here rather than doing in-place calculation. Uses memory, but simpler. 
	if (bmp->Header.Orientation == 0) // origin in lower-left
		{
			UCHAR * tmpData;
			tmpData = (UCHAR*) malloc( bmp->Header.Width* bmp->Header.Height * 4); /* forcing RGBA output*/
			if ( tmpData == NULL )
				{
					BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
					return BMP_OUT_OF_MEMORY;	
				}
			int ascend = 0;
			int descend = 0;
			for (i=(bmp->Header.Height)-1; i>-1; --i) 
				{
					ascend = abs(bmp->Header.Height-i-1)*bmp->Header.Width*4;
					descend = i*bmp->Header.Width*4;
					memcpy(tmpData+ascend, bmp->Data+descend, bmp->Header.Width*4);
				}
			memcpy(bmp->Data,tmpData,bmp->Header.Height*bmp->Header.Width*4);
			free(tmpData);
		}
	data_dst = bmp->Data; 


	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_WIDTH, &PROP_UINT(BMP_GetWidth(bmp)));
	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_HEIGHT, &PROP_UINT(BMP_GetHeight(bmp)));
	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_STRIDE, &PROP_UINT(4 * BMP_GetWidth(bmp)));	

	

	//produce output packet using memory allocation
	pck_dst = gf_filter_pck_new_alloc(stack->dst_pid, BMP_GetWidth(bmp)*BMP_GetHeight(bmp)*4, &data_dst);
	//pck_dst = gf_filter_pck_new_alloc(stack->dst_pid, size, &data_dst);
	if (!pck_dst) return GF_OUT_OF_MEM;
	//memcpy(data_dst, data_src, size);

	//no need to adjust data framing
	//	gf_filter_pck_set_framing(pck_dst, GF_TRUE, GF_TRUE);

	//copy over src props to dst
	gf_filter_pck_merge_properties(pck, pck_dst);
	gf_filter_pck_send(pck_dst);

	gf_filter_pid_drop_packet(stack->src_pid);
	return GF_OK;

}

static GF_Err BMP1BPP_filter_config_input(GF_Filter *filter, GF_FilterPid *pid, Bool is_remove)
{
	const GF_PropertyValue *format;
	GF_PropertyValue p;
	GF_BaseFilter  *stack = (GF_BaseFilter *) gf_filter_get_udta(filter);

	if (stack->src_pid==pid) {
		//disconnect of src pid (not yet supported)
		if (is_remove) {
				if (stack->dst_pid)
		{
			gf_filter_pid_remove(stack->src_pid);
			stack->dst_pid = NULL;
		}
		stack->src_pid = NULL;
		return GF_OK;
		}
		//update of caps, check everything is fine
		else {
			if (!gf_filter_pid_check_caps(pid))
				return GF_NOT_SUPPORTED;
		}
		return GF_OK;
	}
	//check input pid properties we are interested in
	//format = gf_filter_pid_get_property(pid, GF_4CC('c','u','s','t') );
	//if (!format || !format->value.string || strcmp(format->value.string, "myformat")) {
	//	return GF_NOT_SUPPORTED;
	//}


	//setup output (if we are a filter not a sink)

	stack->dst_pid = gf_filter_pid_new(filter);
	gf_filter_pid_copy_properties(stack->dst_pid, stack->src_pid);

	// added these
	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_CODECID, &PROP_UINT(GF_CODECID_RAW));
	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_STREAM_TYPE, &PROP_UINT(GF_STREAM_VISUAL));
	gf_filter_pid_set_property(stack->dst_pid, GF_PROP_PID_PIXFMT, & PROP_UINT( GF_PIXEL_RGB ));
	gf_filter_set_name(filter, "BMP1BPP");

	p.type = GF_PROP_UINT;
	p.value.uint = 10;
	gf_filter_pid_set_property(stack->dst_pid, GF_4CC('c','u','s','2'), &p);

	//set framing mode if needed - by default all PIDs require complete data blocks as inputs
	//	gf_filter_pid_set_framing_mode(pidctx->src_pid, GF_TRUE);

	return GF_OK;
}

static GF_Err base_filter_update_arg(GF_Filter *filter, const char *arg_name, const GF_PropertyValue *arg_val)
{
	return GF_OK;
}

GF_Err base_filter_initialize(GF_Filter *filter)
{
	GF_BaseFilter *stack = gf_filter_get_udta(filter);
	if (stack->opt2) {
		//do something based on options

	}
	//if you filter is a source, this is the right place to start declaring output PIDs, such as above

	return GF_OK;
}

#define OFFS(_n)	#_n, offsetof(GF_BaseFilter, _n)
static const GF_FilterArgs ExampleFilterArgs[] =
{
	//example uint option using enum, result parsed ranges from 0(=v1) to 2(=v3)
	{ OFFS(opt1), "Example option 1", GF_PROP_UINT, "v1", "v1|v2|v3", 0},
	{ OFFS(opt2), "Example option 2", GF_PROP_BOOL, "false", NULL, 0},
	{ NULL }
};

static const GF_FilterCapability BMP1BPPFullCaps[] =
{
	CAP_UINT(GF_CAPS_INPUT, GF_PROP_PID_STREAM_TYPE, GF_STREAM_FILE),
	CAP_STRING(GF_CAPS_INPUT, GF_PROP_PID_FILE_EXT, "bmp"),
	CAP_STRING(GF_CAPS_INPUT, GF_PROP_PID_MIME, "image/bmp"),
	CAP_UINT(GF_CAPS_OUTPUT, GF_PROP_PID_STREAM_TYPE, GF_STREAM_VISUAL),
	CAP_UINT(GF_CAPS_OUTPUT, GF_PROP_PID_CODECID, GF_CODECID_RAW),
};

const GF_FilterRegister BMP1BPPRegister = {
	.name = "BMP1BPP",
	GF_FS_SET_DESCRIPTION("BMP 1BPP")
	GF_FS_SET_HELP("Accessor filter for BMP 1BPP images.")
	.private_size = sizeof(GF_BaseFilter),
	.args = NULL,
	.initialize = base_filter_initialize,
	.finalize = base_filter_finalize,
	SETCAPS(BMP1BPPFullCaps),
	.process = BMP1BPP_filter_process,
	.configure_pid = BMP1BPP_filter_config_input
	
	
};

const GF_FilterRegister * EMSCRIPTEN_KEEPALIVE dynCall_BMP1BPP_register(GF_FilterSession *session)
{
	return &BMP1BPPRegister;
}



