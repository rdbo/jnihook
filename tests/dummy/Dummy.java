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

class YetAnotherClass {
    public static YetAnotherClass instance = new YetAnotherClass(10);

    private int secretNumber;

    private ChildClass childObj;

    public YetAnotherClass(int secretNumber) {
        this.secretNumber = secretNumber;
        this.childObj = new ChildClass();
    }

    public YetAnotherClass(YetAnotherClass obj) {
        this.secretNumber = obj.secretNumber;
        this.childObj = new ChildClass();
        this.childObj.someLetter = obj.childObj.someLetter;
    }

    public static YetAnotherClass getInstance() {
        return instance;
    }

    public int getSecretNumber() {
        return this.secretNumber;
    }

    public char getLetter() {
        return this.childObj.someLetter;
    }

    public static YetAnotherClass copy() {
        return new YetAnotherClass(getInstance());
    }

    class ChildClass {
        char someLetter = 'A';
    }
}

public class Dummy implements Thing {
    private class InternalSecretDummy {
        String secret = "this is super hidden!!!";
    }

    InternalSecretDummy superSecret;

    @Description(value = "This is a field")
    public static int myNumberField = 10;

    public Dummy() {
        System.out.println("Constructor called!!!");
    }

    public static void sayHello() {
        Dummy d = new Dummy();
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

        YetAnotherClass yetAnotherObj = YetAnotherClass.copy();
        System.out.println("=>> OMG!!!! The secret number is: " + yetAnotherObj.getSecretNumber());

        while (true) {
            sayHello();
            sayHi();
            Thread.sleep(1000);
        }
    }

    public void doNothing() {
        
    }
}
