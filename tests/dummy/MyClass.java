package dummy;

public class MyClass {
    private String myname;

    public MyClass(String myname) {
        this.myname = myname;
    }

    public void printName(int n) {
        for (int i = 0; i < n; ++i) {
            System.out.println(this.myname);
        }
    }
}
