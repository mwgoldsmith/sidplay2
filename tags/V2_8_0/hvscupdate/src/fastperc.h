#ifndef fastperc_h
#define fastperc_h

#include <iomanip.h>
#include <iostream.h>

class fastPercent
{
 public:
	
    void init(int max, bool inChokeCheck = false)
    {
        onePercent = nextPercentMark = max/100.0;
        percent = prevPercent = 0;
        chokeCheck = inChokeCheck;
	};

    fastPercent(int max, bool inChokeCheck = false)
    {
        init(max,inChokeCheck);
    };

    void update(int current)
    {
        while (current >= nextPercentMark)
        {
            if (percent < 100)
                percent++;
            nextPercentMark += onePercent;
        }
    };

    bool changed(void)
    {
        return (prevPercent!=percent);
    };

    int get(void)
    {
        if (chokeCheck && onePercent>=512)
        {
            if (prevPercent != percent)
            {
                prevPercent = percent;
            }
        }
        else
        {
            if ( prevPercent <= percent )
            {
                prevPercent = percent + 10;
            }
        }
        return percent;
    };

    void cout(void)
    {
        ::cout << dec << setw(3) << setfill(' ') << percent << "%" << flush;
    };
	
    void coutUpdate(void)
    {
        prevPercent = percent;
        ::cout << "\b\b\b\b";
        cout();
    };

    void end(void)
    {
        percent = 100;
        coutUpdate();
    };

 protected:
	
    float onePercent, nextPercentMark;
    int percent, prevPercent;
    bool chokeCheck;
};

#endif  // fastperc_h
