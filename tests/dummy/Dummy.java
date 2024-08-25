package dummy;

import jdk.jfr.Description;

interface Thing {
    void doNothing();
}

class AnotherClass {
    int number;

    public int getNumber() {
        System.out.println("-> ORIGINAL GET NUMBER CALLED");
    	return this.number;
    }

    public void setNumber(int number) {
        System.out.println("-> ORIGINAL SET NUMBER CALLED");
    	this.number = number;
    }

    public void alsoDoNothing() {
          
    }
}

public class Dummy implements Thing {
    @Description(value = "This is a field")
    public static int myNumberField = 10;

    public static void sayHello() {
        System.out.println("Hello!!! This is the original function!!!");
    }

    public static void sayHi() {
        System.out.println("Hi!!! This is the original function!!!");
    }

    public static void main(String[] args) throws InterruptedException {
        System.out.println("Started Dummy Process");

        sayHello();

        if (args.length == 0) {
            System.out.println("Missing library path!");
        } else {
            System.out.println("Loading library...");
            System.load(args[0]);
        }

        Thread.sleep(1000);

        AnotherClass obj = new AnotherClass();
        obj.setNumber(10);
        System.out.println("-> My Number: " + obj.getNumber());

        while (true) {
            sayHello();
            sayHi();
            Thread.sleep(1000);
        }
    }

    public void doNothing() {
        
    }
}
