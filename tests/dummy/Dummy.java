package dummy;

import jdk.jfr.Description;

interface Thing {
    void doNothing();
}

public class Dummy implements Thing {
    @Description(value = "This is a field")
    public static int myNumberField = 10;

    public static void sayHello() {
        System.out.println("Hello!!! This is the original function!!!");
    }

    public static void main(String[] args) throws InterruptedException {
        System.out.println("Started Dummy Process");

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        Thread.sleep(500);

        sayHello();
    }

    public void doNothing() {
        
    }
}
