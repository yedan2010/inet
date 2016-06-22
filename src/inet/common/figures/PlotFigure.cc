//
// Copyright (C) 2016 OpenSim Ltd
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "PlotFigure.h"
#include "InstrumentUtil.h"

// for the moment commented out as omnet cannot instatiate it from a namespace
using namespace inet;
// namespace inet {

Register_Class(PlotFigure);

#if OMNETPP_VERSION >= 0x500

#define M_PI 3.14159265358979323846
static const char *INIT_PLOT_COLOR = "blue";
static const double TICK_LENGTH = 5;
static const double NUMBER_SIZE_PERCENT = 0.1;
static const double NUMBER_DISTANCE_FROM_TICK = 0.04;

static const char *PKEY_TICK_SIZE = "tickSize";
static const char *PKEY_TIME_WINDOW = "timeWindow";
static const char *PKEY_TIME_TICK_SIZE = "timeTickSize";
static const char *PKEY_LINE_COLOR = "lineColor";
static const char *PKEY_MIN_VALUE= "minValue";
static const char *PKEY_MAX_VALUE = "maxValue";
static const char *PKEY_POS = "pos";
static const char *PKEY_SIZE = "size";
static const char *PKEY_ANCHOR = "anchor";
static const char *PKEY_BOUNDS = "bounds";

PlotFigure::PlotFigure(const char *name) : cGroupFigure(name)
{
    addChildren();
}

const cFigure::Rectangle& PlotFigure::getBounds() const
{
    return bounds;
}

void PlotFigure::setBounds(const Rectangle& rect)
{
    if(bounds == rect)
        return;

    bounds = rect;
    layout();

}

double PlotFigure::getTickSize() const
{
    return tickSize;
}

void PlotFigure::setTickSize(double size)
{
    if(tickSize == size)
        return;

    tickSize = size;
    layout();
}

double PlotFigure::getTimeWindow() const
{
    return timeWindow;
}

void PlotFigure::setTimeWindow(double timeWindow)
{
    if(this->timeWindow == timeWindow)
        return;

    this->timeWindow = timeWindow;
    refresh();
}

double PlotFigure::getTimeTickSize() const
{
    return timeTickSize;
}

void PlotFigure::setTimeTickSize(double size)
{
    if(timeTickSize == size)
        return;

    timeTickSize = size;
    refresh();
}

const cFigure::Color& PlotFigure::getLineColor() const
{
    return plotFigure->getLineColor();
}

void PlotFigure::setLineColor(const Color& color)
{
    plotFigure->setLineColor(color);
}

double PlotFigure::getMinValue() const
{
    return min;
}

void PlotFigure::setMinValue(double value)
{
    if(min == value)
        return;

    min = value;
    layout();
}

double PlotFigure::getMaxValue() const
{
    return max;
}

void PlotFigure::setMaxValue(double value)
{
    if(max == value)
        return;

    max = value;
    layout();
}


void PlotFigure::parse(cProperty *property) {
    cGroupFigure::parse(property);

    setBounds(parseBounds(property));

    const char *s;
    if ((s = property->getValue(PKEY_TICK_SIZE)) != nullptr)
        setTickSize(atoi(s));
    if ((s = property->getValue(PKEY_TIME_WINDOW)) != nullptr)
        setTimeWindow(atoi(s));
    if ((s = property->getValue(PKEY_TIME_TICK_SIZE)) != nullptr)
        setTimeTickSize(atoi(s));
    if ((s = property->getValue(PKEY_LINE_COLOR)) != nullptr)
       setLineColor(parseColor(s));
    if ((s = property->getValue(PKEY_MIN_VALUE)) != nullptr)
       setMinValue(atof(s));
    if ((s = property->getValue(PKEY_MAX_VALUE)) != nullptr)
       setMaxValue(atof(s));

    refresh();
}

const char **PlotFigure::getAllowedPropertyKeys() const
{
    static const char *keys[32];
    if (!keys[0]) {
        const char *localKeys[] = {PKEY_TICK_SIZE, PKEY_TIME_WINDOW, PKEY_TIME_TICK_SIZE,
                                   PKEY_LINE_COLOR, PKEY_MIN_VALUE, PKEY_MAX_VALUE, PKEY_POS, PKEY_SIZE,
                                   PKEY_ANCHOR, PKEY_BOUNDS, nullptr};
        concatArrays(keys, cGroupFigure::getAllowedPropertyKeys(), localKeys);
    }
    return keys;
}

void PlotFigure::addChildren()
{
    plotFigure = new cPathFigure("plot");
    labelFigure = new cLabelFigure("label");
    axisX = new cLineFigure("axisX");
    axisY = new cLineFigure("axisY");

    plotFigure->setLineColor(INIT_PLOT_COLOR);

    addFigure(plotFigure);
    addFigure(labelFigure);
    addFigure(axisX);
    addFigure(axisY);
}

void PlotFigure::setValue(int series, simtime_t timestamp, double value)
{
    ASSERT(series == 0);

    values.push_front(std::pair<simtime_t, double>(timestamp, value));
    refresh();
}

void PlotFigure::layout()
{
    axisY->setStart(Point(bounds.x + bounds.width, bounds.y));
    axisY->setEnd(Point(bounds.x + bounds.width, bounds.y + bounds.height));

    axisX->setStart(Point(bounds.x, bounds.y + bounds.height));
    axisX->setEnd(Point(bounds.x + bounds.width, bounds.y + bounds.height));

    redrawYTicks();
}

void PlotFigure::redrawYTicks()
{
    int numTicks = std::abs(max - min) / tickSize + 1;

    // Allocate ticks and numbers if needed
    if(numTicks > ticksY.size())
        while(numTicks > ticksY.size())
        {
            cLineFigure *tick = new cLineFigure();
            cTextFigure *number = new cTextFigure();

            number->setAnchor(ANCHOR_W);
            number->setFont(Font("", bounds.height * NUMBER_SIZE_PERCENT));

            addFigureBelow(tick, plotFigure);
            addFigureBelow(number, plotFigure);
            ticksY.push_back(Tick(tick, number));
        }
    else
        // Add or remove figures from canvas according to previous number of ticks
        for(int i = ticksY.size() - 1; i >= numTicks; --i)
        {
            delete removeFigure(ticksY[i].number);
            delete removeFigure(ticksY[i].tick);
            ticksY.pop_back();
        }

    for(int i = 0; i < ticksY.size(); ++i)
    {
        double x = bounds.x + bounds.width;
        double y = bounds.y + bounds.height - bounds.height*(i*tickSize)/std::abs(max-min);
        ticksY[i].tick->setStart(Point(x, y));
        ticksY[i].tick->setEnd(Point(x - TICK_LENGTH, y));

        char buf[32];
        sprintf(buf, "%g", min + i*tickSize);
        ticksY[i].number->setText(buf);
        ticksY[i].number->setPosition(Point(ticksY[i].tick->getStart().x + bounds.height * NUMBER_DISTANCE_FROM_TICK, ticksY[i].tick->getStart().y));
     }
}

void PlotFigure::refreshDisplay()
{
    refresh();
}

void PlotFigure::redrawXTicks()
{
    simtime_t minX = simTime() - timeWindow;

    double fraction = std::abs(fmod((minX / timeTickSize).dbl(), 1));
    double shifting;

    if(fraction < std::numeric_limits<double>::epsilon())
        shifting = 0;
    else
        shifting = timeTickSize*(1 - fraction);

    int numTimeTicks = (timeWindow - shifting) / timeTickSize + 1;

    // Allocate ticks and numbers if needed
    if(numTimeTicks > ticksX.size())
        while(numTimeTicks > ticksX.size())
        {
            cLineFigure *tick = new cLineFigure();
            cTextFigure *number = new cTextFigure();

            number->setAnchor(ANCHOR_N);
            number->setFont(Font("", bounds.height * NUMBER_SIZE_PERCENT));

            addFigureBelow(tick, plotFigure);
            addFigureBelow(number, plotFigure);
            ticksX.push_back(Tick(tick, number));
        }
    else
        // Add or remove figures from canvas according to previous number of ticks
        for(int i = ticksX.size() - 1; i >= numTimeTicks; --i)
        {
            delete removeFigure(ticksX[i].number);
            delete removeFigure(ticksX[i].tick);
            ticksX.pop_back();
        }

    for(int i = 0; i < ticksX.size(); ++i)
    {
        double x = bounds.x + bounds.width*(i*timeTickSize + shifting)/std::abs(timeWindow);
        double y = bounds.y + bounds.height;
        ticksX[i].tick->setStart(Point(x, y));
        ticksX[i].tick->setEnd(Point(x, y - TICK_LENGTH));

        // TODO not works with negative numbers
        char buf[32];
        sprintf(buf, "%g", minX.dbl() + i*timeTickSize + shifting);
        ticksX[i].number->setText(buf);
        ticksX[i].number->setPosition(Point(x, y + bounds.height * NUMBER_DISTANCE_FROM_TICK));
     }
}

void PlotFigure::refresh()
{
    // TODO tick X number;

    // ticksX
    redrawXTicks();

    // plot
    simtime_t minX = simTime() - timeWindow;
    simtime_t maxX = simTime();

    plotFigure->clearPath();

    if(values.size() < 2)
        return;
    if(minX > values.front().first)
    {
        values.clear();
        return;
    }

    auto r = getBounds();
    auto it = values.begin();
    double startX = r.x + r.width - (maxX - it->first).dbl() / timeWindow * r.width;
    double startY = r.y + (max - it->second) / std::abs(max - min) * r.height;
    plotFigure->addMoveTo(startX, startY);

    ++it;
    do {
        double endX = r.x + r.width - (maxX - it->first).dbl() / timeWindow * r.width;
        double endY = r.y + (max - it->second) / std::abs(max - min) * r.height;

        double originalStartX = startX;
        double originalStartY = startY;
        double originalEndX = endX;
        double originalEndY = endY;
        if(InstrumentUtil::CohenSutherlandLineClip(startX, startY, endX, endY, r.x, r.x + r.width, r.y, r.y + r.height))
        {
            if(originalStartX != startX || originalStartY != startY)
                plotFigure->addMoveTo(startX, startY);

            plotFigure->addLineTo(endX, endY);
        }

        if(minX > it->first)
            break;

        startX = originalEndX;
        startY = originalEndY;
        ++it;
    } while(it != values.end());

    // Delete old elements
    if(it != values.end())
        values.erase(++it, values.end());
}

#endif // omnetpp 5

// } // namespace inet
