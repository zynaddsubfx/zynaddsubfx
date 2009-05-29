#include <string>

#ifndef CONTROL_H
#define CONTROL_H
/**A control for a parameter within the program*/
class Control
{
    public:
        Control(char ndefaultval);/**\todo create proper initialization list*/
        ~Control(){};
        /**Return the string, which represents the internal value
         * @return a string representation of the current value*/
        virtual std::string getString()const=0;
        /**Set the Control to the given value
         * @param nval A number 0-127*/
        virtual void setmVal(char nval)=0;
        /**Return the midi value (aka the char)
         * @return the current value*/
        virtual char getmVal()const=0;
        /** Will lock the Control until it is ulocked
         *
         * Locking a Control will Stop it from having
         * its internal data altered*/
        void lock();
        /** Will unlock the Control
         * 
         * This will also update the Control
         * if something attempted to update it while it was locked*/
        void ulock();
    private:
        char defaultval;/**<Default value of the Control*/
        char lockqueue; /**<The value is that is stored, while the Control is locked
                         * and something attempts to update it*/
        bool locked;//upgrade this to a integer lock level if problems occur
};

#endif

