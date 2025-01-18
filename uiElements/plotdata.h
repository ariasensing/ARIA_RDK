#ifndef PLOTDATA_H
#define PLOTDATA_H
#include <qwt_plot_curve.h>
#include <plotdescriptor.h>
#include "octavews.h"
#include <qwt_symbol.h>

#include <QwtColorMap>
#include <QwtPlotSpectrogram>
#include <QwtScaleWidget>
#include <QwtScaleDraw>
#include <QwtPlotZoomer>
#include <QwtPlotPanner>
#include <QwtPlotLayout>
#include <QwtPlotRenderer>
#include <QwtInterval>
#include <QwtPainter>
typedef QPair<size_t,double*>								  ArraySize;
typedef QPair<QwtPlotCurve*,ArraySize>						  Curve;
typedef QVector<Curve>										  PlotCurves;
class QwtPlot;

enum QwtPlot_MinMaxUpdate {CUSTOM=0, FULL=1, MAXHOLD=2};

class plotData
{
private:
	PLOT_TYPE       _ptype;
	octavews*		_ws;
	QwtPlot*		_owner;
public:
	plotData(octavews* ws=nullptr, QwtPlot* parent=nullptr, PLOT_TYPE pt = PT_NONE) {_ws = ws; _ptype = pt; _owner = parent;}
	~plotData() {};
	// Simple plots
	virtual void set_data_plot(QString var_name, QString x_name) {};
	virtual void set_data_plot(QStringList var_name, QString x_name) {};
	virtual void set_data_plot(QString var_name, QString x_name, QString y_name) {};
	virtual void clean_data() {};
	inline octavews* workspace() {return _ws;}
	inline QwtPlot* parent()  {return _owner;}
	virtual bool		has_var_in_list(const QStringList& var_changed) {return false;}
	virtual bool		has_var_in_list(const QString& var_changed) {return false;}
//	virtual bool		has_var_in_list(std::set<std::string>& var_list) {return false;}
	PLOT_TYPE			get_plot_type() {return _ptype;}
	virtual std::map<QString,PlotCurves>		get_curves() {return std::map<QString,PlotCurves>();}
	virtual	bool update_data(const std::set<std::string>& varlist) {return false;};
	virtual bool update_data() {return false;};
	virtual double get_xmax() {return 1.0;}
	virtual double get_xmin() {return -1.0;}
	virtual double get_ymax() {return 1.0;}
	virtual double get_ymin() {return -1.0;}
	virtual void   update_min_max(QwtPlot_MinMaxUpdate policy = FULL) {};
};

class plotData_plot : public plotData
{
private:

	std::map<QString, PlotCurves>				    curves;
	std::map<QString, ArraySize>					xvals;	// We store the number of elements, too
	std::map<QString,QString>						x_y;
	Qt::GlobalColor  gc;
	Qt::PenStyle	 style;
	QwtSymbol::Style symbol;
	QwtSymbol		 *objSymbol;

	double _xmin, _xmax, _ymin, _ymax;

	bool	manage_data_curve  (const QString& yname, const QString& xname ="", size_t ncurves=1, size_t npt=0);
	bool	fill_data_y(QString vname, const NDArray& data);
	bool	fill_data_x(QString vname, const NDArray& data);
public:
	plotData_plot(octavews* ws = nullptr, QwtPlot* parent=nullptr) : plotData(ws,parent,PTQWT_PLOT), curves(), xvals() {gc =  gc = Qt::red; style = Qt::SolidLine; symbol = QwtSymbol::Ellipse; objSymbol=nullptr;}
	~plotData_plot();
	void    update_min_max(QwtPlot_MinMaxUpdate policy = FULL) override;
	void set_data_plot(QString var_name, QString x_name) override;
	void set_data_plot(QStringList var_name, QString x_name) override;
	void set_data_plot(QString var_name, QString x_name, QString y_name) override {};
	bool update_data(const std::set<std::string>& varlist) override;
	bool update_data()  override;
	void clean_data()   override;
	bool		has_var_in_list(const QStringList& var_changed) override;
	bool		has_var_in_list(const QString& var_changed) override;
//	bool		has_var_in_list(std::set<std::string>& var_list) override;
	std::map<QString,PlotCurves>		get_curves() override {return curves;}

	double get_xmax() override {return _xmax*(_xmax > 0 ? 1.1 : 0.9);}
	double get_xmin() override {return _xmin*(_xmin > 0 ? 0.9 : 1.1);}
	double get_ymax() override {return _ymax*(_ymax > 0 ? 1.1 : 0.9);}
	double get_ymin() override {return _ymin*(_ymin > 0 ? 0.9 : 1.1);}

};



class LinearColorMap : public QwtLinearColorMap
{
public:
	LinearColorMap( int formatType )
		: QwtLinearColorMap( Qt::darkCyan, Qt::red )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		addColorStop( 0.1, Qt::cyan );
		addColorStop( 0.6, Qt::green );
		addColorStop( 0.95, Qt::yellow );
	}
};

class HueColorMap : public QwtHueColorMap
{
public:
	HueColorMap( int formatType )
		: QwtHueColorMap( QwtColorMap::Indexed )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		//setHueInterval( 240, 60 );
		//setHueInterval( 240, 420 );
		setHueInterval( 0, 359 );
		setSaturation( 150 );
		setValue( 200 );
	}
};

class SaturationColorMap : public QwtSaturationValueColorMap
{
public:
	SaturationColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 0, 255 );
		setValueInterval( 255, 255 );
	}
};

class ValueColorMap : public QwtSaturationValueColorMap
{
public:
	ValueColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 255, 255 );
		setValueInterval( 70, 255 );
	}
};

class SVColorMap : public QwtSaturationValueColorMap
{
public:
	SVColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		setHue( 220 );
		setSaturationInterval( 100, 255 );
		setValueInterval( 70, 255 );
	}
};

class AlphaColorMap : public QwtAlphaColorMap
{
public:
	AlphaColorMap( int formatType )
	{
		setFormat( ( QwtColorMap::Format ) formatType );

		//setColor( QColor("DarkSalmon") );
		setColor( QColor("SteelBlue") );
	}
};
//-------------------------------------------------------------
// Data for density graphs
class plotData_Density : public QwtRasterData, public plotData
{
private:
	QPair <QString, ArraySize> xaxis;
	QPair <QString, ArraySize> yaxis;
	//double**
	double _xmin, _xmax, _ymin, _ymax;
	QwtInterval m_intervals[3];
public:
	plotData_Density(octavews* ws = nullptr, QwtPlot* parent=nullptr);
	~plotData_Density();
	void    update_min_max(QwtPlot_MinMaxUpdate policy = FULL) override;
	void set_data_plot(QString var_name, QString x_name) override {};
	void set_data_plot(QStringList var_name, QString x_name) override {};
	void set_data_plot(QString var_name, QString x_name, QString y_name) override;
	bool update_data(const std::set<std::string>& varlist) override;
	bool update_data()  override;
	void clean_data()   override;
	bool		has_var_in_list(const QStringList& var_changed) override;
	bool		has_var_in_list(const QString& var_changed) override;
	//	bool		has_var_in_list(std::set<std::string>& var_list) override;
	std::map<QString,PlotCurves>		get_curves() override {return plotData::get_curves();}

	double get_xmax() override {return _xmax*(_xmax > 0 ? 1.1 : 0.9);}
	double get_xmin() override {return _xmin*(_xmin > 0 ? 0.9 : 1.1);}
	double get_ymax() override {return _ymax*(_ymax > 0 ? 1.1 : 0.9);}
	double get_ymin() override {return _ymin*(_ymin > 0 ? 0.9 : 1.1);}

	virtual QwtInterval interval( Qt::Axis axis ) const QWT_OVERRIDE;
	virtual double value( double x, double y ) const QWT_OVERRIDE;



};
#endif // PLOTDATA_H
