#ifndef QPREFERNCESDIALOG_HPP
#define QPREFERNCESDIALOG_HPP

#include "ui_preferences.h"
#include "qviewercore.hpp"

namespace isis
{
namespace viewer
{

class QPreferencesDialog : public QDialog
{
	Q_OBJECT;

public:
	QPreferencesDialog( QWidget *parent, QViewerCore *core );
protected:
	Ui::preferencesDialog preferencesUi;


public Q_SLOTS:
	void closeEvent( QCloseEvent * );
	void loadSettings();
	void saveSettings();
	void apply(int dummy = 0);
	virtual void showEvent( QShowEvent * ) { loadSettings(); }

private:
	QViewerCore *m_ViewerCore;

};


}
}



#endif