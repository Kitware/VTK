/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
 ** ** If you wish to add, delete or rename slots use Qt Designer which will **
 update this file, preserving your code. Create an init() slot in place of ** a
 constructor, and a destroy() slot in place of a destructor.
 *****************************************************************************/

#include "metrics.hpp" 
#include "quadmetrics.hpp" 
#include "trimetrics.hpp"
#include "tetmetrics.hpp"

void PlotterForm::init() 
{ 
  int i; 
  for(i=0; Metric::ElementTypes[i].name != 0; i++) 
  {
    QString name = Metric::ElementTypes[i].name;
    ElementTypeCombo->insertItem(name); 
  } 
  propogate_metrics(0); 
}
    
    
void PlotterForm::propogate_metrics(int which_element) 
{ 
  MetricCombo->clear();
  const Metric::metric_funcs* metrics =
    Metric::ElementTypes[which_element].functions; 
  int i; 
  for(i=0; metrics[i].name != 0; i++) 
  { 
    QString name = metrics[i].name; 
    MetricCombo->insertItem(name); 
  } 
}


void PlotterForm::do_plot() 
{

  int color_factor = ColorFactor->value(); 
  color_factor = color_factor*color_factor/4 + 1;
  Metric::set_color_factor(color_factor); 
  Metric *metric = 0; 
  int curr_element_type = ElementTypeCombo->currentItem(); 
  int curr_metric = MetricCombo->currentItem(); 
  if( strcmp(Metric::ElementTypes[curr_element_type].name, "quad") == 0 ) 
  { 
    VerdictFunction metric_function =
      Metric::ElementTypes[curr_element_type].functions[curr_metric].func;
    metric = new Metric2DQuad(metric_function);
    connect(metric, SIGNAL(current_val_changed()), this, SLOT(update_metric_val() ) );
    mPlotter->set_metric(metric);
  }
  else if( strcmp(Metric::ElementTypes[curr_element_type].name, "quad (3d)") == 0 ) 
  { 
    VerdictFunction metric_function =
      Metric::ElementTypes[curr_element_type].functions[curr_metric].func;
    metric = new Metric3DQuad(metric_function);
    connect(metric, SIGNAL(current_val_changed()), this, SLOT(update_metric_val() ) );
    mPlotter->set_metric(metric);
  }
  else if( strcmp(Metric::ElementTypes[curr_element_type].name, "tri") == 0 )
  {
    VerdictFunction metric_function = Metric::ElementTypes[curr_element_type].functions[curr_metric].func;
    metric = new Metric2DTri(metric_function);
    connect(metric, SIGNAL(current_val_changed()), this, SLOT(update_metric_val() ) );
    mPlotter->set_metric(metric);
  }
  else if( strcmp(Metric::ElementTypes[curr_element_type].name, "tet") == 0 )
  {
    VerdictFunction metric_function = Metric::ElementTypes[curr_element_type].functions[curr_metric].func;
    metric = new Metric3DTet(metric_function);
    connect(metric, SIGNAL(current_val_changed()), this, SLOT(update_metric_val() ) );
    mPlotter->set_metric(metric);
  }

}



void PlotterForm::update_metric_val()
{
    double metric_val = Metric::curr_metric_val();
    QString value;
    value.setNum(metric_val);
    MetricVal->setText(value);   
}


void PlotterForm::ZValChanged(int val)
{
    int metricZVal = (int)((double) val / 100.0 * (double)(NUM_Z_PLANES-1) );
    Metric *metric = mPlotter->get_metric();
    if(metric)
    {
        float realval = metric->setZVal(metricZVal);
        QString num;
        num.setNum(realval);
        RealZVal->setText(num);
    }
    else
    {
        QString num;
        num.setNum(0.0);
        RealZVal->setText(num);
    }
}
