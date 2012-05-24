#ifndef NIO_H
#define NIO_H
#include <string>
#include <set>

/**Interface to Nio Subsystem
 *
 * Should be only externally included header */
namespace Nio
{
    void init(void);
    bool start(void);
    void stop(void);

    void setDefaultSource(std::string name);
    void setDefaultSink(std::string name);

    bool setSource(std::string name);
    bool setSink(std::string name);

    void setPostfix(std::string post);
    std::string getPostfix(void);

    std::set<std::string> getSources(void);
    std::set<std::string> getSinks(void);

    std::string getSource(void);
    std::string getSink(void);

    //Get the prefered sample rate from jack (if running)
    void preferedSampleRate(unsigned &rate);

    extern bool autoConnect;
    extern std::string defaultSource;
    extern std::string defaultSink;
};

#endif
