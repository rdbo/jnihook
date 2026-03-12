package dummy;

import java.io.IOException;
import java.lang.Thread;
class Target {
    public static class TargetSubclass {
        public static void doWhatever() {
            System.out.println("do whatever");
        }
    }
    public Target() {
        System.out.println("constructor");
    }
    public static Target newTarget() {
        return new Target();
    }

    public static Target returnTarget(Target t) {
        return t;
    }

    public void say(String msg) {
        System.out.println(msg);
    }

    public static void sayAnotherThing(int number) {
        System.out.println("Another thing from Target object!");
        System.out.println("My number: " + number);
    }

    public void sayHello() {
        Runnable a = new Runnable() {
            public void run() {
                System.out.println("runnable here");
            }
        };
        a.run();
        TargetSubclass.doWhatever();

        // Force bad case for jnihook
        Object obj = (Object)this;
        Target obj2 = (Target)obj;

        obj2.say("Hello from Target object!");

        sayAnotherThing(10);
    }
    public int midFunctionTest(){
        System.out.println("guess what");
        System.out.println("what");
        return 10;
    }
    public static void midFunctionTest2(){
        System.out.println("blah blah");
        System.out.println("something");
    }
}


public class Dummy {
    public static void main(String[] args) throws IOException {
        System.out.println();
        System.out.println("Press [ENTER] to begin testing...");
        System.out.println("(wait for the library to load!)");
        System.out.println();

        if (args.length == 0) {
            System.out.println("Missing library path!");
            System.exit(-1);
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        System.in.read();
        Target obj = new Target();
        Target target = Target.returnTarget(Target.newTarget());
        target.sayHello();
        target.sayHello();
        target.midFunctionTest();
        Target.midFunctionTest2();
        Target.midFunctionTest2();

        System.out.println("Done!");
    }
}
