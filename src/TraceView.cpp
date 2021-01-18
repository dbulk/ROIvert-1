#include <TraceView.h>
#include "opencv2/opencv.hpp"
#include "widgets/TraceChartWidget.h"
#include "roivertcore.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QApplication>
#include <QPushButton>

#include <QDebug>
//**** note always that traceid is 1 based, it's the roi number not the 

struct TraceView::pimpl {
    pimpl();
    void setParent(TraceView* p) { parent = p; }

    void setData(cv::Mat* datasource) { data = datasource;  } 

    void updateTraces(std::vector<size_t> ids);

    size_t getDataSize() { 
        return data==nullptr ? 0 : data->size().height; 
    };

    void setSelected(size_t traceid); 
    size_t getSelected(){ return selected; }

    QVBoxLayout* lay = new QVBoxLayout;
    QScrollArea* scroll = new QScrollArea;
    TraceChartWidget* RidgeChart = new TraceChartWidget;
    std::vector<TraceChartWidget*> LineCharts; 

    float timelimits[2] = { 0,1 };
    
private:
    TraceView* parent;

    cv::Mat* data;
    size_t selected = 0;

    QColor selectedcolor = Qt::green; 
    QColor unselectedcolor = Qt::red; 
    bool usePalette; 
};

TraceView::TraceView(cv::Mat* DataSource, QWidget* parent)
    : QWidget(parent) {

    QGridLayout* glay = new QGridLayout;
    setLayout(glay);
    QTabWidget* tab = new QTabWidget;
    glay->addWidget(tab);

    QWidget* tabLine = new QWidget;
    QWidget* tabRidgeLine = new QWidget;
    QWidget* tabImage = new QWidget;

    tab->addTab(tabLine, "Line");
    tab->addTab(tabRidgeLine, "Ridge");
    tab->addTab(tabImage, "Image");
    tab->setTabEnabled(2, false);

    // tabLine -> lay -> impl.scroll -> scrollAreaContent -> impl.lay -> charts!
    //  can move some of this into impl constructor
    {
        auto lay = new QGridLayout; 
        tabLine->setLayout(lay);
        auto scrollAreaContent = new QWidget;
        lay->addWidget(impl->scroll);
        scrollAreaContent->setLayout(impl->lay);
        impl->scroll->setWidget(scrollAreaContent);
        impl->scroll->setWidgetResizable(true);
        //impl->scroll->setContentsMargins(0, 0, 0, 0);
        lay->setContentsMargins(0, 0, 0, 0);
        impl->lay->setContentsMargins(0, 0, 0, 0);
    }

    {
        auto lay = new QGridLayout;
        tabRidgeLine->setLayout(lay);
        lay->addWidget(impl->RidgeChart);
    }
    impl->setData(DataSource);
    impl->setParent(this);

    connect(impl->RidgeChart, &TraceChartWidget::clicked, this, [=](QPointF pos, int clickind) {
        select(clickind + 1);
        emit traceSelected(clickind + 1);
    });

}

TraceView::~TraceView() = default;

void TraceView::updateTraces(size_t traceid, bool down){
    // note that we can probably just update everything always...
    size_t sz = down ? impl->getDataSize() - traceid + 1 : 1;
    sz = std::clamp(sz, (size_t)1, impl->getDataSize());

    std::vector<size_t> tracelist(sz);
    std::iota(tracelist.begin(), tracelist.end(), traceid);
    impl->updateTraces(tracelist);
}

void TraceView::removeTrace(size_t traceid) {
    if (traceid == impl->getSelected()) { 
        // this is probably always true because how do you delete something that's unselected?
        impl->setSelected(0); 
    }
    updateTraces(traceid, true);
}

void TraceView::select(size_t traceid) { impl->setSelected(traceid); update(); }
size_t TraceView::getSelected() { return impl->getSelected(); }

void TraceView::connectChartSelect(TraceChartWidget* chart, size_t traceid) {
    connect(chart, &TraceChartWidget::clicked, this, [=]() {select(traceid); emit traceSelected(traceid); });
}
void TraceView::setTimeLimits(float min, float max) {
    impl->timelimits[0] = min;
    impl->timelimits[1] = max; 
    updateTraces(0, true); 
}

//
namespace {
    void initLineChart(TraceChartWidget* chart, int id) {
        chart->setXLabel("Time (s)");
        chart->setTitle("ROI " + QString::number(id));
        chart->setContentsMargins(0, 0, 0, 0);
        chart->setMinimumHeight(350);
        chart->setYLabel(ROIVert::dffstring());
    }
    void initRidgeChart(TraceChartWidget* chart) {
        auto style = chart->getStyle();
        style.Axis.ShowY = false;
        style.Line.FillGradientDivisor = 2;
        style.Line.FillGradientExponent = 1;
        style.Line.Colors = TraceChart::paletteColors();
        chart->setStyle(style);
        chart->setContentsMargins(0, 0, 0, 0);
        chart->setXLabel("Time (s)");
    }
}

TraceView::pimpl::pimpl() {
    initRidgeChart(RidgeChart);
}

void TraceView::pimpl::updateTraces(std::vector<size_t> ids) {
    // assumes sorted ascending
    for (auto& id : ids) {
        // if there's no chart at this id, add one
        if (id > LineCharts.size()) {
            LineCharts.push_back(new TraceChartWidget);
            lay->addWidget(LineCharts[id - 1]);
            initLineChart(LineCharts[id - 1], id);
            parent->connectChartSelect(LineCharts[id-1], id);
        }
        if (id - 1 < data->size().height) {
            cv::Mat thisrow = data->row(id - 1);
            LineCharts[id - 1]->setData(thisrow, "trace", timelimits[0], timelimits[1]);
            RidgeChart->setData(data->row(id - 1), "ROI " + QString::number(id), timelimits[0], timelimits[1], -(float)id / 2., TraceChart::NORM::ZEROTOONE);
        }
    }

    // remove any charts that aren't in data:
    for (int i = getDataSize(); i < LineCharts.size(); ++i) {
        delete LineCharts[i];
        LineCharts.erase(LineCharts.begin() + i);
        RidgeChart->removeData("ROI " + QString::number(i + 1));
    }
};

void TraceView::pimpl::setSelected(size_t newselect) {
    // update colors:
    if (selected > 0 && selected <= LineCharts.size()) {
        // there was already one selected, unselect it:
        auto cs = LineCharts[selected - 1]->getStyle();
        cs.Line.Colors = { unselectedcolor };
        LineCharts[selected - 1]->setStyle(cs);
        LineCharts[selected - 1]->update();
    }

    if (newselect > 0) {
        if (newselect <= LineCharts.size()) {
            auto chart = LineCharts[newselect - 1];

            auto cs = chart->getStyle();
            cs.Line.Colors = { selectedcolor };
            chart->setStyle(cs);
            chart->update();

            // update scroll 
            if (newselect == LineCharts.size()) {
                // (needs two updates (????) on add for scroll to update?):
                qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
                qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
            }
            
            scroll->ensureVisible(0, chart->y() + chart->height(), 0, 0); //bottom
            scroll->ensureVisible(0, chart->y(), 0, 0); // top
        }
        // update select
        selected = newselect;
    }
}