template <typename T>
class VotingBuffer
{
    public:
        
    void initialise(int len) 
    {
        if (buffer != nullptr) {
            delete[] buffer; 
        }

        buffer_len = len;
        buffer = new T[len];

    }

    void update(T val)
    {
        buffer[nextIndex] = val;
        if (nextIndex == buffer_len - 1){nextIndex = 0;}
        else{nextIndex++;}
        if (current_len < 5) { current_len++; };
    }
    
    // Boyer-Moore Majority Voting
    // https://www.geeksforgeeks.org/theory-of-computation/boyer-moore-majority-voting-algorithm/
    int findMajority()
    {
        int iteration_len = buffer_len;
        if (current_len < buffer_len) { iteration_len = current_len; }
        int i, candidate = -1, votes = 0;
        // Finding majority candidate
        for (i = 0; i < iteration_len; i++) {
            if (votes == 0) {
                candidate = buffer[i];
                votes = 1;
            }
            else {
                if (buffer[i] == candidate)
                    votes++;
                else
                    votes--;
            }
        }
        int count = 0;
        for (i = 0; i < iteration_len; i++) {
            if (buffer[i] == candidate)
                count++;
        }

        if (count > iteration_len / 2)
            return candidate;
        return -1;
    }
        
    private:
        int buffer_len;
        int nextIndex = 0;
        int current_len = 0;
        T* buffer = nullptr;
};
