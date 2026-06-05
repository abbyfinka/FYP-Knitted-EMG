template <typename T>
class EMA
{
    public:
        
    void begin(T alpha) 
    {
        m_alpha = alpha;
        m_lastValue = T(0);
    }

    T filter(T input)
    {
        m_lastValue = m_alpha * input + (T(1) - m_alpha) * m_lastValue;
        return m_lastValue;
    }
        
    private:
        T m_alpha;
        T m_lastValue;
};
