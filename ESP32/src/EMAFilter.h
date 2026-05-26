template <typename T>
class EMAFilter
{
  public:
    
  void begin(T alpha) {
      m_alpha = alpha;
      m_lastOutput = T(0);
  }
  T filter(T input);
      
  private:
    T m_alpha;
    T m_lastOutput;
};

template <typename T>
T EMAFilter<T>::filter(T input)
{
    m_lastOutput = m_alpha * input + (T(1) - m_alpha) * m_lastOutput;
    return m_lastOutput;
}