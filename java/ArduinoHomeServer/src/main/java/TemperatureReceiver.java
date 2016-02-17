import java.text.DateFormat;
import java.util.Locale;

import de.wyraz.home.temp.TemperatureReceiverThread;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorType;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorValueListener;



public class TemperatureReceiver
{
    public static void main(String[] args) throws Exception
    {
        SensorValueListener tl=new SensorValueListener() {
            public void setSensorValue(String source, SensorType type, String key, float value, int sequenceNumber)
            {
                System.err.println(DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.MEDIUM,Locale.GERMANY).format(System.currentTimeMillis())+" "+type+" "+key+": "+value+" (#"+sequenceNumber+")");
            }
        };
        new TemperatureReceiverThread(tl).start();
    }

}
