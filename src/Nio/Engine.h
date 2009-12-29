#ifndef ENGINE_H
#define ENGINE_H
#include <string>
/**Marker for input/output driver*/
class Engine
{
    public:
        Engine();
        virtual ~Engine();
        std::string name;
};
#endif
