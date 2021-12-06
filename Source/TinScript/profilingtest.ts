// --------------------------------------------------------------------------------------------------------------------
// ProfilingTest.ts
// Demo script for profiling
// --------------------------------------------------------------------------------------------------------------------

Print("Welcome to profilingtest.ts");

// --------------------------------------------------------------------------------------------------------------------
// -- Iterative
int fibonacci_i(int n)
{
    int prev = 0;
    int fib = 1;
    int i;
    for (i = 1; i < n; ++i)
    {
        fib = fib + prev;
        prev = fib - prev;
    }
    return (fib);
}

// -- elapsed time
void fib_test(int iterations)
{
    int cur_time = GetSimTime();
    Print("cur time: ", cur_time);
    int s = 0;
        int i;
        for (i = 1; i <= iterations; ++i)
        {
                fibonacci_i(50);
        }
        int elapsed = GetSimTime() - cur_time;
    Print("elapsed time: ", elapsed);
}

// --------------------------------------------------------------------------------------------------------------------
hashtable vector_list;
void vector_test(int count, vector3f pos)
{
    int cur_time = GetSimTime();
        object vector_list = create CObjectGroup();
        int i;
        for (i = 0; i < count; ++i)
        {
                vector3f v;
                v:x = -1000.0f + (Random() * 2000.0f);
                v:y = -1000.0f + (Random() * 2000.0f);
                v:z = -1000.0f + (Random() * 2000.0f);
                
                object vector_obj = create CVector3f();
                vector_obj.v = v;
                vector_list.AddObject(vector_obj);
        }
        int elapsed = GetSimTime() - cur_time;
    Print("Table elapsed time: ", elapsed);
        
        vector3f closest;
        float closest_dist = 1000000.0f;
        vector_obj = vector_list.First();
        while (IsObject(vector_obj))
        {
                v = vector_obj.v;
                float length = V3fLength(v);
                if (length < closest_dist)
                {
                        closest = v;
                        closest_dist = length;
                }
                vector_obj = vector_list.Next();
        }
        
        Print(closest:x, " ", closest:y, " ", closest:z, " len: ", V3fLength(closest));
        destroy vector_list;
        elapsed = GetSimTime() - cur_time;
    Print("Total elapsed time: ", elapsed);
}

// --------------------------------------------------------------------------------------------------------------------
int CallFromCode(int a, int b, string foo)
{
    float neg_pi = -pi;
    float sum = 0;
    int i = 0;
    for (i = 0; i < 10000; ++i)
    {
        sum += abs(-pi);
    }

    Print("### Call From Code() result: ", sum);
}

int CallMe()
{
    return (57);
}

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------