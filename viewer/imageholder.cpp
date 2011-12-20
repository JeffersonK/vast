
#include "imageholder.hpp"

namespace isis
{
namespace viewer
{

ImageHolder::ImageHolder() {}

boost::numeric::ublas::matrix< double > ImageHolder::getNormalizedImageOrientation( bool transposed ) const
{
	boost::numeric::ublas::matrix<double> retMatrix = boost::numeric::ublas::zero_matrix<double>( 4, 4 );
	retMatrix( 3, 3 ) = 1;
	const util::fvector4 &rowVec = m_Image->propertyValue( "rowVec" )->castTo<util::fvector4>();
	const util::fvector4 &columnVec = m_Image->propertyValue( "columnVec" )->castTo<util::fvector4>();
	const util::fvector4 &sliceVec = m_Image->propertyValue( "sliceVec" )->castTo<util::fvector4>();
	size_t rB = rowVec.getBiggestVecElemAbs();
	size_t cB = columnVec.getBiggestVecElemAbs();
	size_t sB = sliceVec.getBiggestVecElemAbs();

	//if image is rotated of 45 �
	if( rB == cB ) {
		if( sB == 0 ) {
			rB = 1;
			cB = 2;
		} else if ( sB == 1 ) {
			rB = 0;
			cB = 2;
		} else if ( sB == 2 ) {
			rB = 0;
			rB = 1;
		}
	}

	if( rB == sB ) {
		if( cB == 0 ) {
			rB = 1;
			sB = 2;
		} else if ( cB == 1 ) {
			rB = 0;
			sB = 2;
		} else if ( cB == 2 ) {
			rB = 0;
			sB = 1;
		}
	}

	if( cB == sB ) {
		if( rB == 0 ) {
			cB = 1;
			sB = 2;
		} else if ( rB == 1 ) {
			cB = 0;
			sB = 2;
		} else if ( rB == 2 ) {
			cB = 0;
			sB = 1;
		}
	}

	if( !transposed ) {
		retMatrix( rB, 0 ) = rowVec[rB] < 0 ? -1 : 1;
		retMatrix( cB, 1 ) = columnVec[cB] < 0 ? -1 : 1;
		retMatrix( sB, 2 ) =  sliceVec[sB] < 0 ? -1 : 1;
	} else {
		retMatrix( 0, rB ) = rowVec[rB] < 0 ? -1 : 1;
		retMatrix( 1, cB ) =  columnVec[cB] < 0 ? -1 : 1;
		retMatrix( 2, sB ) = sliceVec[sB] < 0 ? -1 : 1;
	}

	return retMatrix;
}

boost::numeric::ublas::matrix< double > ImageHolder::getImageOrientation( bool transposed ) const
{
	boost::numeric::ublas::matrix<double> retMatrix = boost::numeric::ublas::zero_matrix<double>( 4, 4 );
	retMatrix( 3, 3 ) = 1;
	const util::fvector4 &rowVec = m_Image->propertyValue( "rowVec" )->castTo<util::fvector4>();
	const util::fvector4 &columnVec = m_Image->propertyValue( "columnVec" )->castTo<util::fvector4>();
	const util::fvector4 &sliceVec = m_Image->propertyValue( "sliceVec" )->castTo<util::fvector4>();

	for ( uint16_t i = 0; i < 3; i++ ) {
		if( !transposed ) {
			retMatrix( i, 0 ) = rowVec[i];
			retMatrix( i, 1 ) = columnVec[i];
			retMatrix( i, 2 ) = sliceVec[i];
		} else {
			retMatrix( 0, i ) = rowVec[i];
			retMatrix( 1, i ) = columnVec[i];
			retMatrix( 2, i ) = sliceVec[i];
		}
	}

	return retMatrix;
}

bool ImageHolder::setImage( const data::Image &image, const ImageType &_imageType, const std::string &filename )
{

	//some checks
	if( image.isEmpty() ) {
		LOG( Runtime, error ) << "Getting an empty image? Obviously something went wrong.";
		return false;
	}

	m_Image.reset( new data::Image( image ) );

	//if no filename was specified we have to search for the filename by ourselfes
	if( filename.empty() ) {
		// go through all the chunks and search for filenames. We use a set here to avoid redundantly filenames
		std::set<std::string> filenameSet;
		BOOST_FOREACH( std::vector< data::Chunk >::const_reference chRef, image.copyChunksToVector() ) {
			filenameSet.insert( chRef.getPropertyAs<std::string>( "source" ) );
		}
		//now we pack our filenameSet into our slist of filenames
		BOOST_FOREACH( std::set<std::string>::const_reference setRef, filenameSet ) {
			m_Filenames.push_back( setRef );
		}
	} else {
		m_Filenames.push_back( filename );
	}

	// get some image information
	
	majorTypeID = image.getMajorTypeID();
	m_ImageSize = image.getSizeAsVector();
	LOG( Debug, verbose_info )  << "Fetched image of size " << m_ImageSize << " and type "
								<< image.getMajorTypeName() << ".";
	//copy the image into continuous memory space and assure consistent data type

	if( data::ValuePtr<util::color24>::staticID != majorTypeID && data::ValuePtr<util::color48>::staticID != majorTypeID ) {
		isRGB = false;
		minMax = image.getMinMax();
		copyImageToVector<InternalImageType>( image );
	} else {
		copyImageToVector<InternalImageColorType>( image );
		isRGB = true;
	}

	LOG_IF( m_ImageVector.empty(), Runtime, error ) << "Size of image vector is 0!";

	if( m_ImageVector.size() != m_ImageSize[3] ) {
		LOG( Runtime, error ) << "The number of timesteps (" << m_ImageSize[3]
							  << ") does not coincide with the number of volumes ("  << m_ImageVector.size() << ").";
		return false;
	}

	//create the chunk vector
	BOOST_FOREACH( std::vector< ImagePointerType >::const_reference pointerRef, m_ImageVector ) {
		m_ChunkVector.push_back( data::Chunk(  pointerRef, m_ImageSize[0], m_ImageSize[1], m_ImageSize[2] ) );
	}

	LOG( Debug, verbose_info ) << "Spliced image to " << m_ImageVector.size() << " volumes.";

	//image seems to be ok...i guess

	//add some more properties
	imageType = _imageType;
	interpolationType = nn;

	if( imageType == z_map ) {
		lowerThreshold = 0;
		upperThreshold = 0;
		lut = std::string( "standard_zmap" );
	} else if( imageType == anatomical_image ) {
		if( !isRGB ) {
			lowerThreshold = minMax.first->as<double>() ;
			lowerThreshold = minMax.second->as<double>();
		}

		lut = std::string( "standard_grey_values" );
	}

	voxelSize = image.getPropertyAs<util::fvector4>( "voxelSize" );

	if( image.hasProperty( "voxelGap" ) ) {
		voxelSize += image.getPropertyAs<util::fvector4>( "voxelGap" );
	}


	voxelCoords = util::ivector4( m_ImageSize[0] / 2, m_ImageSize[1] / 2, m_ImageSize[2] / 2, 0 );
	physicalCoords = m_Image->getPhysicalCoordsFromIndex( voxelCoords );
	isVisible = true;
	opacity = 1.0;
	scaling = 1.0;
	offset = 0.0;
	colorMap = util::Singletons::get<color::Color, 10>().getColormapMap().at( lut );

	if( !isRGB ) {
		extent = fabs( minMax.second->as<double>() - minMax.first->as<double>() );
		optimalScalingOffset = getOptimalScaling();
		
		m_PropMap.setPropertyAs<double>( "scalingMinValue", minMax.first->as<double>() );
		m_PropMap.setPropertyAs<double>( "scalingMaxValue", minMax.second->as<double>() );
        scalingToInternalType = image.getScalingTo( isis::data::ValuePtr<InternalImageType>::staticID );
	}

	alignedSize32 = get32BitAlignedSize( m_ImageSize );
	m_PropMap.setPropertyAs<bool>( "init", true );
	m_PropMap.setPropertyAs<util::slist>( "changedAttributes", util::slist() );
	updateOrientation();
	m_PropMap.setPropertyAs<util::fvector4>( "originalColumnVec", image.getPropertyAs<util::fvector4>( "columnVec" ) );
	m_PropMap.setPropertyAs<util::fvector4>( "originalRowVec", image.getPropertyAs<util::fvector4>( "rowVec" ) );
	m_PropMap.setPropertyAs<util::fvector4>( "originalSliceVec", image.getPropertyAs<util::fvector4>( "sliceVec" ) );
	m_PropMap.setPropertyAs<util::fvector4>( "originalIndexOrigin", image.getPropertyAs<util::fvector4>( "indexOrigin" ) );
	updateColorMap();
	return true;
}

void ImageHolder::addChangedAttribute( const std::string &attribute )
{
	util::slist attributes = m_PropMap.getPropertyAs<util::slist>( "changedAttributes" );
	attributes.push_back( attribute );
	m_PropMap.setPropertyAs<util::slist>( "changedAttributes", attributes );
}

bool ImageHolder::removeChangedAttribute( const std::string &attribute )
{
	util::slist attributes = m_PropMap.getPropertyAs<util::slist>( "changedAttributes" );
	util::slist::iterator iter = std::find( attributes.begin(), attributes.end(), attribute );

	if( iter == attributes.end() ) {
		return false;
	} else {
		attributes.erase( iter );
		m_PropMap.setPropertyAs<util::slist>( "changedAttributes", attributes );
		return true;
	}
}

std::pair< double, double > ImageHolder::getOptimalScaling()
{
	const float lowerCutOff = 0.01;
	const float upperCutOff = 0.01;
	const size_t volume = getImageSize()[0] * getImageSize()[1] * getImageSize()[2];
	const InternalImageType minImage = internMinMax.first->as<InternalImageType>();
	const InternalImageType maxImage = internMinMax.second->as<InternalImageType>();
	const InternalImageType extent = maxImage - minImage;
	double *nHistogram = ( double * ) calloc( extent + 1, sizeof( double ) );
	histogramVector.resize( getImageSize()[3] );

	for( unsigned short t = 0; t < getImageSize()[3]; t++ ) {
		histogramVector[t] = ( double * ) calloc( extent + 1, sizeof( double ) ) ;
		InternalImageType *dataPtr = static_cast<InternalImageType *>( getImageVector()[t]->getRawAddress().get() );
		//create the histogram
		#pragma omp parallel for

		for( size_t i = 0; i < volume; i++ ) {
			histogramVector[t][dataPtr[i]]++;
		}
	}

	//normalize histogram
	#pragma omp parallel for

	for( InternalImageType i = 0; i < extent; i++ ) {
		nHistogram[i] = histogramVector.front()[i] / volume;
	}

	InternalImageType upperBorder = extent - 1;
	InternalImageType lowerBorder = 0;
	double sum = 0;

	while( sum < upperCutOff ) {
		sum += nHistogram[upperBorder--];

	}

	sum = 0;

	while ( sum < lowerCutOff ) {
		sum += nHistogram[lowerBorder++];
	}

	std::pair<double, double> retPair;
	retPair.first = lowerBorder;
	retPair.second = ( float )std::numeric_limits<InternalImageType>::max() / float( upperBorder - lowerBorder );
	delete[] nHistogram;
	return retPair;
}



///calls isis::data::Image::updateOrientationMatrices() and sets latchedOrientation and orientation of the isis::viewer::ImageHolder
void ImageHolder::updateOrientation()
{
	m_Image->updateOrientationMatrices();
	latchedOrientation = getNormalizedImageOrientation();
	orientation = getImageOrientation();
}

void ImageHolder::checkVoxelCoords( util::ivector4 &vc )
{
	for( unsigned short i = 0; i < 4; i++ ) {
		vc[i] = vc[i] < 0 ? 0 : vc[i];
		vc[i] = vc[i] >= static_cast<int>( getImageSize()[i] ) ? static_cast<int>( getImageSize()[i] ) - 1 : vc[i]; //cast to avoid warning

	}
}

void ImageHolder::updateColorMap()
{
	if( !isRGB ) {
		util::Singletons::get<color::Color, 10>().adaptColorMapToImage( this );
	}
}

void ImageHolder::removeWidget( WidgetInterface *widget )
{
	std::list<WidgetInterface *>::iterator iter = std::find( m_WidgetList.begin(), m_WidgetList.end(), widget );

	if( iter != m_WidgetList.end() ) {
		m_WidgetList.erase( iter );
	}
}

void ImageHolder::syncImage()
{
	// first check if sizes coincide 
	if( isRGB ) {
		LOG( Runtime, warning ) << "Not yet capable of syncronizing rgb images.";
		return;
	}
	
	bool coincide = true;
	for ( unsigned short i = 0; i < 4; i++ ) {
		if( getImageSize()[i] != getISISImage()->getSizeAsVector()[i] ) {
			coincide = false;
		}
	}
	if( !coincide ) {
		LOG( Runtime, error ) << "The size of the ImageHolder and the nested isis image do not coincide. Can not sync!";
		return;
	}
	
	if( coincide && !isRGB ) {
		switch( majorTypeID ) {
			case data::ValuePtr<bool>::staticID:
				_syncImage<bool>();
				break;
			case data::ValuePtr<uint8_t>::staticID:
				_syncImage<uint8_t>();
				break;
			case data::ValuePtr<int8_t>::staticID:
				_syncImage<int8_t>();
				break;
			case data::ValuePtr<uint16_t>::staticID:
				_syncImage<uint16_t>();
				break;				
			case data::ValuePtr<int16_t>::staticID:
				_syncImage<int16_t>();
				break;
			case data::ValuePtr<uint32_t>::staticID:
				_syncImage<uint32_t>();
				break;
			case data::ValuePtr<int32_t>::staticID:
				_syncImage<int32_t>();
				break;
			case data::ValuePtr<uint64_t>::staticID:
				_syncImage<uint64_t>();
				break;
			case data::ValuePtr<int64_t>::staticID:
				_syncImage<int64_t>();
				break;
			case data::ValuePtr<float>::staticID:
				_syncImage<float>();
				break;
			case data::ValuePtr<double>::staticID:
				_syncImage<double>();
				break;
		}
	}
	
}


}
} //end namespace

