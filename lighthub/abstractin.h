class Input;
class abstractIn {
public:
    abstractIn(Input * _in){in=_in;};
    virtual int Setup(int addr) = 0;
    virtual int Poll() = 0;

protected:
   Input * in;
friend Input;
};
