package de.wyraz.home.web;

import java.io.IOException;
import java.io.PrintWriter;
import java.text.DateFormat;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.JSONException;
import org.json.JSONObject;

import de.wyraz.home.temp.TemperatureReceiverThread;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorType;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorValueListener;
import tools.SSLTrustCa;

public class TemperatureServlet extends HttpServlet implements SensorValueListener
{
	static {
        SSLTrustCa.trustLetsEncryptRootCa();
	}
	
    protected TemperatureReceiverThread trt;
    protected MqttClient mqtt;
//    protected HttpClient client;

//    protected InfluxDB influxDB;
//    protected String dbName = "home";
    
    @Override
    public void init() throws ServletException
    {
        super.init();
//        InfluxDB influxDB = InfluxDBFactory.connect("https://ts.wyraz.de/db/","home","home");
//    	influxDB.createDatabase(dbName);
//    	influxDB.enableBatch(2000, 100, TimeUnit.MILLISECONDS);
//    	this.influxDB=influxDB;
//        client = new HttpClientImpl("https://tsdb.wyraz.de/");
        
        try
        {
			mqtt = new MqttClient("tcp://192.168.1.3", "legacy-gw");
			MqttConnectOptions options = new MqttConnectOptions();
			options.setUserName("username");
			options.setPassword("password".toCharArray());
			options.setAutomaticReconnect(true);
			mqtt.connect(options);
		}
        catch (MqttException e)
        {
			e.printStackTrace();
		}
    	
        trt=new TemperatureReceiverThread(this);
        trt.start();
    }
    @Override
    public void destroy()
    {
        super.destroy();
        trt.shutdown();
//        influxDB.disableBatch();
    }
    
    protected Map<String,Value> sensorValues=new HashMap<String, TemperatureServlet.Value>();
    
    protected final Set<String> sensorsAussen=new HashSet(Arrays.asList("temp-49914","temp-9781","temp-36771","temp-32527"));

    // ds18s20-1706 - Pool (alt, 2polig)
    // ds18s20-9781 - Schuppen außen
    // ds18s20-8675 - Schuppen innen
    // ds18s20-22317 - Lab innen
    // ds18s20-32527 - Lab außen
    // ds18s20-36771 - Schuppen außen
    // ds18s20-43501 - Server
    // ds18s20-49914 - Schuppen außen
    // ds18s20-57728 - Pool neu
    
    public void setSensorValue(String source, SensorType type, String key, float value, int sequenceNumber)
    {
    	setSensorValue(source, type, key, value, sequenceNumber, null);
    }
    
    public void setSensorValue(String source, SensorType type, String key, float value, int sequenceNumber, String extraInfo)
    {
        System.err.println(DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.MEDIUM,Locale.GERMANY)
        		.format(System.currentTimeMillis())+" "+source+" "+type+" "+key+": "+value+" (#"+sequenceNumber+")");
        Value val=new Value();
        val.type=type;
        val.value=value;
        val.ts=System.currentTimeMillis();
        val.extraInfo=extraInfo;
        sensorValues.put(key, val);
        
        NumberFormat nf=NumberFormat.getInstance(Locale.ENGLISH);
        nf.setMaximumFractionDigits(1);
        nf.setGroupingUsed(false);
        
        try
        {
			MqttMessage message = new MqttMessage(nf.format(value).getBytes());
	        switch (type)
	        {
	        	case TEMPERATURE:
	        		mqtt.publish("sensor/"+key+"/temp", message);
	        		break;
	        	case HUMIDITY:
	        		mqtt.publish("sensor/"+key+"/humid", message);
	        		break;
	        }
        }
        catch (Exception ex)
        {
        	ex.printStackTrace();
        }
        
//        MetricBuilder builder = MetricBuilder.getInstance();
//        builder.addMetric("home."+val.type.name())
//        	.setDataPoint(System.currentTimeMillis(), value).addTag("key", key);
//        try {
//            Response response = client.pushMetrics(builder, ExpectResponse.SUMMARY);
////            System.out.println(response);
//        } catch (IOException e) {
//            e.printStackTrace();
//        }

        
//        Point point = Point.measurement(val.type.name())
//                .time(System.currentTimeMillis(), TimeUnit.MILLISECONDS)
//                .field(key,value)
//                .build();
//        influxDB.write(dbName, "default", point);

        
        if (sensorsAussen.contains(key))
        {
            Float minValue=null;
            int count=0;
            for (String k: sensorsAussen)
            {
                Value v=getValue(k,false);
                if (v!=null) count++;
                if (v!=null && (minValue==null || minValue>v.value)) minValue=v.value;
            }
            if (minValue!=null) {
            	setSensorValue("min(aussen)", SensorType.TEMPERATURE, "aussenMin", minValue, 0,count+"/"+sensorsAussen.size());
            }
        }
        
    }
    
    protected static class Value
    {
        SensorType type;
        float value;
        long ts;
        boolean outdated;
        String extraInfo;
    }

    protected String getFormattedValue(String key)
    {
        Value val=getValue(key,true);
        
        if (val==null) return "---";
        
        String result=getFormattedValue(val);
        if (val.outdated) result="("+result+")";
        if (val.extraInfo!=null) result+=" ("+val.extraInfo+")";
    	return result;
        
    }
    
    protected String getFormattedValue(Value val)
    {
        if (val.type==SensorType.TEMPERATURE||val.type==SensorType.REMOTE_SWITCH_MIN_TEMP)
        {
            NumberFormat nf=NumberFormat.getInstance(Locale.GERMANY);
            int digits=(val.type==SensorType.REMOTE_SWITCH_MIN_TEMP)?0:1;
            nf.setMinimumFractionDigits(digits);
            nf.setMaximumFractionDigits(digits);
            return nf.format(val.value)+" °C";
        }
        
        if (val.type==SensorType.HUMIDITY)
        {
            NumberFormat nf=NumberFormat.getInstance(Locale.GERMANY);
            nf.setMinimumFractionDigits(1);
            nf.setMaximumFractionDigits(1);
            return nf.format(val.value)+" %";
        }
        
        if (val.type==SensorType.REMOTE_SWITCH_OUT)
        {
        	return (val.value==0)?"AUS":"EIN";
        }
        
        if (val.type==SensorType.BRIGHTNESS)
        {
            NumberFormat nf=NumberFormat.getIntegerInstance(Locale.GERMANY);
            nf.setGroupingUsed(false);
            return nf.format(val.value);
        }
        
        NumberFormat nf=NumberFormat.getInstance(Locale.GERMANY);
        nf.setMaximumFractionDigits(2);
	    return nf.format(val.value);
    }
    protected Value getValue(String key, boolean getOutdatedAsWell)
    {
        Value val=sensorValues.get(key);
        if (val!=null)
        {
            long maxAgeInMinutes;
            switch (val.type)
            {
                case REMOTE_SWITCH_MIN_TEMP:
                    maxAgeInMinutes=60;
                    break;
                case REMOTE_SWITCH_OUT:
                    maxAgeInMinutes=5;
                    break;
                case TEMPERATURE:
                case HUMIDITY:
                    maxAgeInMinutes=15;
                    break;
                case BRIGHTNESS:
                    maxAgeInMinutes=5;
                    break;
                default:
                    maxAgeInMinutes=10;
                    break;
            }
            if (val.ts+(60000*maxAgeInMinutes)<System.currentTimeMillis())
            {
                if (getOutdatedAsWell) val.outdated=true;
                else val=null;
            }
        }
        return val;
    }
    
    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException
    {
        doGet(req, resp);
    }
    
    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException
    {
        if ("/".equals(req.getPathInfo()))
        {
            resp.setContentType("text/html");
            resp.setCharacterEncoding("utf-8");
            PrintWriter writer=resp.getWriter();
            writer.println("<html><body><table>");
            for (String key:new TreeSet<String>(this.sensorValues.keySet()))
            {
                writer.println("<tr><td>"+key+":</td><td>"+getFormattedValue(key)+"</td></tr>");
            }
            writer.println("</table></body></html>");
            writer.close();
            return;
        }
        if ("/val".equals(req.getPathInfo()))
        {
            resp.setContentType("text/plain");
            PrintWriter writer=resp.getWriter();
            Value val=getValue(req.getParameter("key"),false);
            if (val!=null) writer.println(val.value);
            else writer.println(" ");
            writer.close();
            return;
        }
        if ("/json".equals(req.getPathInfo()))
        {
            resp.setContentType("text/plain");
            resp.setCharacterEncoding("utf-8");
            try
            {
                JSONObject result=new JSONObject();
                for (String key:new TreeSet<String>(this.sensorValues.keySet()))
                {
                    Value value=getValue(key, true);
                    if (value==null) continue;
                    JSONObject val=new JSONObject();
                        val.put("key", key);
                        val.put("value", value.value);
                        val.put("ts", value.ts);
                        val.put("outdated", value.outdated);
                    result.put(key, val);
                }
                {
                    result.put("datetime",
                            DateFormat.getDateTimeInstance(
                                    DateFormat.LONG, DateFormat.SHORT, Locale.GERMANY)
                                        .format(System.currentTimeMillis()));
                }
                PrintWriter writer=resp.getWriter();
                writer.print(result.toString(2));
            }
            catch (JSONException ex)
            {
                ex.printStackTrace();
            }
            return;
        }
        if ("/rss".equals(req.getPathInfo()))
        {
            resp.setContentType("text/plain");
            resp.setCharacterEncoding("utf-8");
            PrintWriter writer=resp.getWriter();
            writer.println("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
            writer.println("<rss version=\"2.0\">");
            writer.println("<channel>");
            writer.println("<title>MW Home</title>");
            writer.println("<item><title>Außen "+new SimpleDateFormat("HH:mm").format(System.currentTimeMillis())+"</title><description>"+getFormattedValue("aussenMin")+"</description></item>");
            writer.println("<item><title>Pool</title><description>"+getFormattedValue("temp-57728")+"</description></item>");
            writer.println("<item><title>Schuppen</title><description>"+getFormattedValue("temp-8675")+"</description></item>");
            writer.println("<item><title>Lab "+getFormattedValue("switch-30A")+" "+getFormattedValue("swtemp-30A")+"</title><description>"+getFormattedValue("temp-22317")+"</description></item>");
            writer.println("<item><title>Wohnz.</title><description>"+getFormattedValue("temp-16")+"/"+getFormattedValue("humi-16")+"</description></item>");
            writer.println("<item><title>Annab.</title><description>"+getFormattedValue("temp-17")+"/"+getFormattedValue("humi-17")+"</description></item>");
            writer.println("<item><title>Felix</title><description>"+getFormattedValue("temp-18")+"/"+getFormattedValue("humi-18")+"</description></item>");
            writer.println("<item><title>Sauna</title><description>"+getFormattedValue("temp-47356")+"</description></item>");
            writer.println("<item><title>Helligkeit</title><description>"+getFormattedValue("b1")+"</description></item>");
            writer.println("</channel>");
            writer.println("</rss>");
            return;
        }
                
        super.doGet(req, resp);
    }
}
