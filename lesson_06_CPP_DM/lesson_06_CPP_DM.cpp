//============================================================================
// Name        : lesson_06_CPP_DM.cpp
// Author      : andry antonenko
// IDE         : Qt Creator 4.14.2 based on Qt 5.15.2
// Description : lesson 06 of the C++: difficult moments course
//============================================================================
#include <QCoreApplication>
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <set>
#include <tuple>
#include <utility>

//----------------------------------------------------------------------------
/*
1. Создайте потокобезопасную оболочку для объекта cout.
Назовите ее pcout. Необходимо,
чтобы несколько потоков могли обращаться к pcout и
информация выводилась в консоль. Продемонстрируйте работу pcout.
//*/

namespace less_06 {
  class pcout
  {
  private:
  static std::mutex m;
  std::stringstream s;
  public:
    pcout()
    {
      pcout::m.lock();
    }
    ~pcout()
    {
      std::cout << s.str();
      pcout::m.unlock();
    }

    template<typename T>
    pcout& operator<<(const T& t)
    {
      s << t;
      return *this;
    }
  };

  std::mutex pcout::m;

  const size_t AMOUNT_CYCLES = 10;

  void printThreadNumber(int number) {
    using namespace std::chrono_literals;
    size_t n = AMOUNT_CYCLES + 1;
    while(--n)
    {
      pcout() << "Thread " << number << "\n";
      std::this_thread::sleep_for(100ms);
    }
  }
}

void task_1()
{
  std::cout << "Task 1\n" << std::endl;

  std::thread th1(less_06::printThreadNumber, 1);
  std::thread th2(less_06::printThreadNumber, 2);
  std::thread th3(less_06::printThreadNumber, 3);
  th1.join();
  th2.join();
  th3.join();

  std::cout << std::endl;
}

//----------------------------------------------------------------------------
/*
2. Реализовать функцию, возвращающую i-ое простое число
(например, миллионное простое число равно 15485863).
Вычисления реализовать во вторичном потоке.
В консоли отображать прогресс вычисления.
//*/

namespace less_06 {

  bool isSimple(const size_t nD)
  {
    size_t nSqrt = (size_t)std::sqrt(nD);

    size_t nDivider;
    size_t nIter = 0;
    if (nD > 3)
    {
      for(nDivider = 2;;nDivider = 2 + nIter)
      {
        if (nIter < 1) nIter++;
        else nIter += 2;

        if ((nD % nDivider) == 0)
        {
          return false;
        }
        else if (nDivider > nSqrt)
        {
          return true;
        }
      }
    }
    else
    {
        return true;
    }
  }

  void getSimpleNumber(size_t n, size_t& result)
  {
    pcout() << "Start thread: " << std::this_thread::get_id() << "\n";

    if(n == 0)
    {
      result = 0;
      return;
    }
    if(n == 1)
    {
      result = 2;
      return;
    }

    size_t cnt { 1 };
    int progress { 0 };
    int oldProgress { 0 };

    for(size_t i { 3 }; i != SIZE_MAX; i += 2)
    {
      if(isSimple(i))
      {
        ++cnt;

        progress = cnt*100/n;
        if(progress != oldProgress)
        {
          oldProgress = progress;
          pcout() << "Progress (tread: " << std::this_thread::get_id()
            << "): " << progress << "%\n";
        }

        if(cnt == n)
        {
          result = i;
          return;
        }
      }
    }
    result = 0;//out of the range
  }
}

void task_2()
{
  std::cout << "\nTask 2\n" << std::endl;

  size_t rez1;
  size_t rez2;
  size_t rez3;

  std::thread th1(less_06::getSimpleNumber, 3, std::ref(rez1));
  std::thread th2(less_06::getSimpleNumber, 7, std::ref(rez2));
  std::thread th3(less_06::getSimpleNumber, 10'000, std::ref(rez3));

  th1.join();
  std::cout << "First rezult: " << rez1 << "\n";

  th2.join();
  std::cout << "Second rezult: " << rez2 << "\n";

  th3.join();
  std::cout << "Third rezult: " << rez3 << "\n";

  std::cout << std::endl;
}

//----------------------------------------------------------------------------
/*
3. Промоделировать следующую ситуацию.
Есть два человека (2 потока): хозяин и вор.
Хозяин приносит домой вещи.
При этом у каждой вещи есть своя ценность.
Вор забирает вещи, каждый раз забирает вещь с наибольшей ценностью.
//*/

namespace less_06 {

  int getRandomNum(int min, int max)
  {
    const static auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    static std::mt19937_64 generator(seed);
    std::uniform_int_distribution<int> dis(min, max);
    return dis(generator);
  }

  class Thing
  {
  private:
    const int WORTH_MAX = {1'000'000};
    const int WORTH_MIN = {0};
    int worth;
  public:
    Thing()
      :worth(getRandomNum(WORTH_MIN,WORTH_MAX)){}
    int getWorth() const {return worth;}
    bool operator<(const Thing& th) const;

  };

  std::ostream& operator<<(std::ostream& out, const Thing &th)
  {
    out << th.getWorth();
    return out;
  }

  bool Thing::operator<(const Thing& th) const
  {
    return worth < th.worth;
  }

  class Home
  {
  private:
    std::set<Thing> Things;
    std::mutex m;
  public:
    Home(){}
    void addThing(Thing& th);
    std::pair<bool, std::shared_ptr<Thing>> extractThingMaxWorth();
    int getMaxWorth() const;
    void deleteThings();
    friend std::ostream& operator<<(std::ostream& out, const Home &th);
  };

  std::ostream& operator<<(std::ostream& out, const Home &th)
  {
    if(th.Things.empty())
    {
      out << "There aren't any things in the home.";
      return out;
    }

    for(const auto& a: th.Things)
      out << a << " ";

    return out;
  }

  void Home::addThing(Thing& th)
  {
    std::lock_guard lg(m);
    Things.insert(std::move(th));
  }

  void Home::deleteThings()
  {
    std::lock_guard lg(m);
    Things.clear();
  }

  std::pair<bool, std::shared_ptr<Thing>> Home::extractThingMaxWorth()
  {
    std::lock_guard lg(m);

    std::shared_ptr<Thing> pthing;
    if (Things.begin() ==  Things.end())
      return std::make_pair(false,pthing);

    auto it { Things.end() };
    --it;
    //Thing th { *it };
    pthing = std::make_shared<Thing>(*it);
    Things.erase(it);
    return std::make_pair(true,pthing);
  }

  int Home::getMaxWorth() const
  {
    auto it = Things.end();
    --it;
    return it->getWorth();
  }

  class Owner
  {
  private:
    std::shared_ptr<Home> pHome;
  public:
    Owner(std::shared_ptr<Home>& aHome)
      :pHome(aHome){}
    void addThingToHome();
  };

  void Owner::addThingToHome()
  {
    Thing th;
    pHome->addThing(th);
  }

  class Thief
  {
  private:
    std::shared_ptr<Home> pHome;
  public:
    Thief(std::shared_ptr<Home>& aHome)
      :pHome(aHome){}
    std::pair<bool, std::shared_ptr<Thing>> theftThing();
  };

  std::pair<bool, std::shared_ptr<Thing>> Thief::theftThing()
  {
    return pHome->extractThingMaxWorth();
  }

  class Modeling
  {
  private:
    int periodAdding;
    int periodThefting;

    std::shared_ptr<Home> pHome;
  public:
    Modeling(const int aPeriodAdding = 1000, const int aPeriodThefting = 500)
      :periodAdding(aPeriodAdding),periodThefting(aPeriodThefting){
      pHome = std::make_shared<Home>();
    }
    void run(const int time);
  };

  void Modeling::run(const int timeModeling)
  {
    Owner* owner { new Owner(pHome) };
    Thief* thief { new Thief(pHome) };

    auto runOwner = [](Owner* owner, int ms_timeModeling, int ms_periodAdding)
    {
      int cycles = ms_timeModeling/ms_periodAdding;
      while(--cycles)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_periodAdding));
        owner->addThingToHome();
      }
    };
    auto runThief = [](Thief* thief, int ms_timeModeling, int ms_periodAdding)
    {
      int cycles = ms_timeModeling/ms_periodAdding;
      while(--cycles)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_periodAdding));
        thief->theftThing();
      }
    };

    std::thread thOwner(runOwner, owner, timeModeling, periodAdding);
    std::thread thThief(runThief, thief, timeModeling, periodThefting);

    thOwner.join();
    thThief.join();

    std::cout << *pHome << std::endl;

    delete owner;
    delete thief;
  }
}

void task_3()
{
  std::cout << "\nTask 3\n" << std::endl;

  const int TIME_MODELING = 10000;//ms
  {
    const int PERIOD_ADDING = 1000;//ms
    const int PERIOD_THEFTING = 500;//ms
    std::cout << "\nTest 1" << std::endl;
    std::cout << "Period of adding: " << PERIOD_ADDING << " ms" << std::endl;
    std::cout << "Period of thefting: " << PERIOD_THEFTING << " ms" << std::endl;
    less_06::Modeling model(PERIOD_ADDING,PERIOD_THEFTING);
    model.run(TIME_MODELING);
  }

  {
    const int PERIOD_ADDING = 500;//ms
    const int PERIOD_THEFTING = 1000;//ms
    std::cout << "\nTest 2" << std::endl;
    std::cout << "Period of adding: " << PERIOD_ADDING << " ms" << std::endl;
    std::cout << "Period of thefting: " << PERIOD_THEFTING << " ms" << std::endl;
    less_06::Modeling model(PERIOD_ADDING,PERIOD_THEFTING);
    model.run(TIME_MODELING);
  }

  std::cout << std::endl;
}

//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  //--------------------------------------------------------------------------
  // Task 1
  /*
  task_1();
  //*/
  //--------------------------------------------------------------------------
  // Task 2
  /*
  task_2();
  //*/
  //--------------------------------------------------------------------------
  // Task 3
  //*
  task_3();
  //*/
  //--------------------------------------------------------------------------
  return a.exec();
}
