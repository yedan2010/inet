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

#ifndef __INET_PLOTFIGURE_H
#define __INET_PLOTFIGURE_H

#include "inet/common/INETDefs.h"
#include "inet/common/INETMath.h"
#include "IIndicatorFigure.h"

// for the moment commented out as omnet cannot instatiate it from a namespace
//namespace inet {

#if OMNETPP_VERSION >= 0x500

class INET_API PlotFigure : public cGroupFigure, public inet::IIndicatorFigure
{
    struct Tick
    {
        cLineFigure *tick;
        cTextFigure *number;

        Tick(cLineFigure *tick, cTextFigure *number) : tick(tick), number(number) {}
    };

    cPathFigure *plotFigure;
    cLabelFigure *labelFigure;
    cLineFigure *axisX, *axisY;
    std::vector<Tick> ticksX;
    std::vector<Tick> ticksY;

    double timeWindow = 10;
    double tickSize = 2.5;
    double timeTickSize = 3;
    double min = 0;
    double max = 10;
    std::list<std::pair<simtime_t, double>> values;
    Rectangle bounds;

  protected:
    void addChildren();
    void layout();
    void refresh();

  public:
    PlotFigure(const char *name = nullptr);
    virtual ~PlotFigure() {};

    virtual void parse(cProperty *property) override;
    const char **getAllowedPropertyKeys() const override;

    virtual void refreshDisplay() override;

    virtual void setValue(int series, simtime_t timestamp, double value) override;

    //getters and setters
    const Rectangle& getBounds() const;
    void setBounds(const Rectangle& rect);

    double getTickSize() const;
    void setTickSize(double size);

    double getTimeWindow() const;
    void setTimeWindow(double timeWindow);

    double getTimeTickSize() const;
    void setTimeTickSize(double size);

    const Color& getLineColor() const;
    void setLineColor(const Color& color);

    double getMinValue() const;
    void setMinValue(double value);

    double getMaxValue() const;
    void setMaxValue(double value);
};

#else

// dummy figure for OMNeT++ 4.x
class INET_API PlotFigure : public cGroupFigure {

};

#endif // omnetpp 5

// } // namespace inet

#endif // ifndef __INET_PLOTFIGURE_H

