using namespace std;

class Point{
    private:
        double x, y;

    public:
        Point();
        Point(double x1, double y1);
        double getX();
        double getY();
        void setX(double val);
        void setY(double val);
        double distance(Point p);
        double distance(double x1, double y1);
};