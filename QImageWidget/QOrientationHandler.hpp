#ifndef QORIENTATIONHANDLER_HPP
#define QORIENTATIONHANDLER_HPP

#include <QTransform>
#include "ImageHolder.hpp"
#include "common.hpp"

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/concept_check.hpp>

namespace isis
{
namespace viewer
{
namespace qt
{

class QOrienationHandler
{

public:
	static util::fvector4 mapCoordsToOrientation( const util::fvector4 &coords, const boost::shared_ptr<ImageHolder> image, PlaneOrientation orientation, bool back = false, bool absolute = true );
	static util::ivector4 getMappedCoords( const boost::shared_ptr<ImageHolder> image, const util::ivector4 &coords );

	static util::fvector4 getScalingAndOffset( const boost::shared_ptr< ImageHolder > image, const size_t &w, const size_t &h, PlaneOrientation orientation );
	static QTransform getTransform( util::PropertyMap &properties, const boost::shared_ptr< ImageHolder > image, const size_t &w, const size_t &h, PlaneOrientation orientation );

	static std::pair<size_t, size_t> convertWindow2VoxelCoords( const util::PropertyMap &properties, const boost::shared_ptr< ImageHolder > image, const size_t &x, const size_t &y, PlaneOrientation orientation );
	static std::pair<size_t, size_t> convertVoxel2WindowCoords( const util::PropertyMap &properties, const boost::shared_ptr< ImageHolder > image, PlaneOrientation orientation );
};


}
}
}// end namespace


#endif