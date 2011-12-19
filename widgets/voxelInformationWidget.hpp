#ifndef VOXELINFORMATIONWIDGET_HPP
#define VOXELINFORMATIONWIDGET_HPP

#include "ui_voxelInformationWidget.h"
#include "common.hpp"
#include "qviewercore.hpp"
#include <QThread>

namespace isis
{
namespace viewer
{
namespace widget
{

	
class VoxelInformationWidget : public QWidget
{
	Q_OBJECT
	class TimePlayThread : public QThread 
	{
		QViewerCore *m_core;
		int m_start;
		int m_end;
		Ui::voxelInformationWidget *m_interface;
		
	public: 
		TimePlayThread( QViewerCore *core, Ui::voxelInformationWidget *interface ) : m_core(core), m_start(0), m_end(0), m_interface(interface) {} ;
		void setStartStop( int start, int stop) { m_start = start; m_end = stop; }
		void run()
		{
			uint16_t deleyTime = m_core->getOptionMap()->getPropertyAs<uint16_t>("timeseriesPlayDelayTime");
			uint16_t t = m_start;
			while(true) {
				t = t == m_end ? 0 : t;
				msleep(deleyTime);
				m_interface->timestepSlider->setValue(t);
				m_interface->timestepSpinBox->setValue(t);
				QApplication::processEvents();
				t++;
				
			}
		}
	};
	
public:
	VoxelInformationWidget( QWidget *parent, QViewerCore *core );

	Ui::voxelInformationWidget &getInterface() { return m_Interface; }

public Q_SLOTS:
	void synchronize();
	void synchronizePos( util::ivector4 voxelCoords );
	void synchronizePos( util::fvector4 physicalCoords );
	void voxPosChanged();
	void physPosChanged();
	void updateLowerUpperThreshold(  );
	void playTimecourse();
	void timePlayFinished();

private:
	isis::viewer::QViewerCore *m_ViewerCore;
	Ui::voxelInformationWidget m_Interface;
	bool isConnected;
	void connectSignals();
	void disconnectSignals();
	void reconnectSignals();
	TimePlayThread *m_tThread;

	template<typename TYPE>
	void displayIntensity( const util::ivector4 &coords ) const {
		const util::Value<TYPE> vIntensity ( m_ViewerCore->getCurrentImage()->getISISImage()->voxel<TYPE>( coords[0], coords[1], coords[2], coords[3] ) );
		const double intensity = roundNumber<double>( vIntensity, 4 );
		m_Interface.intensityValue->setText( QString::number( intensity ) );
	}

	template<typename TYPE>
	void displayIntensityColor( const util::ivector4 &coords ) const {
		util::checkType<TYPE>();
		const util::Value<TYPE> vIntensity ( m_ViewerCore->getCurrentImage()->getISISImage()->voxel<TYPE>( coords[0], coords[1], coords[2], coords[3] ) );
		const std::string intensityStr = static_cast<const util::_internal::ValueBase &>( vIntensity ).as<std::string>();
		m_Interface.intensityValue->setText( intensityStr.c_str() );
	}
	
};

}
}
}




#endif