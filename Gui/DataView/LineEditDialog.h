/**
 * Copyright (c) 2012, OpenGeoSys Community (http://www.opengeosys.net)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.net/LICENSE.txt
 *
 * \file LineEditDialog.h
 *
 * Created on 2010-12-09 by Karsten Rink
 */

#ifndef LINEEDITDIALOG_H
#define LINEEDITDIALOG_H

#include "ui_LineEdit.h"
#include <QDialog>

#include "PolylineVec.h"

class QStringListModel;

/**
 * \brief A dialog window for manipulation of polylines.
 * Currently included functionality is the concatenation of polylines
 * as well as creating polygons or surfaces from polylines.
 */
class LineEditDialog : public QDialog, private Ui_LineEdit
{
	Q_OBJECT

public:
	LineEditDialog(const GeoLib::PolylineVec &ply_vec, QDialog* parent = 0);
	~LineEditDialog(void);

private:
	std::vector<size_t> getSelectedIndeces(QStringList list);

	QStringListModel* _allPly;
	QStringListModel* _selPly;
	std::string _geoName;

private slots:
	/// Instructions when polylines are selected.
	void on_selectPlyButton_pressed();

	/// Instructions when polylines are deselected.
	void on_deselectPlyButton_pressed();

	/// Instructions if the OK-Button has been pressed.
	void accept();

	/// Instructions if the Cancel-Button has been pressed.
	void reject();

signals:
	void connectPolylines(const std::string&,
	                      std::vector<size_t>,
	                      double,
	                      std::string,
	                      bool,
	                      bool);
	void triangulateSurface(const GeoLib::Polyline);
};

#endif //LINEEDITDIALOG_H
