package de.wyraz.home.web;

import java.io.IOException;
import java.io.PrintWriter;
import java.security.KeyStore.SecretKeyEntry;
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

import org.json.JSONException;
import org.json.JSONObject;

import de.wyraz.home.temp.TemperatureReceiverThread;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorType;
import de.wyraz.home.temp.TemperatureReceiverThread.SensorValueListener;

public class TemperatureServlet extends HttpServlet implements SensorValueListener
{
    protected TemperatureReceiverThread trt;
    
    @Override
    public void init() throws ServletException
    {
        super.init();
        trt=new TemperatureReceiverThread(this);
        trt.start();
    }
    @Override
    public void destroy()
    {
        super.destroy();
        trt.shutdown();
    }
    
    protected Map<String,Value> sensorValues=new HashMap<String, TemperatureServlet.Value>();
    
    protected final Set<String> sensorsAussen=new HashSet(Arrays.asList("ds18s20-49914","ds18s20-9781","ds18s20-36771","ds18s20-32527"));

    // ds18s20-1706 - Pool (alt, 2polig)
    // ds18s20-9781 - Schuppen außen
    // ds18s20-8675 - Schuppen innen
    // ds18s20-22317 - Lab innen
    // ds18s20-32527 - Lab außen
    // ds18s20-36771 - Schuppen außen
    // ds18s20-43501 - Server
    // ds18s20-49914 - Schuppen außen
    // ds18s20-57728 - Pool neu
    
    public void setSensorValue(SensorType type, String key, float value, int sequenceNumber)
    {
        System.err.println(DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.MEDIUM,Locale.GERMANY).format(System.currentTimeMillis())+" "+type+" "+key+": "+value+" (#"+sequenceNumber+")");
        Value val=new Value();
        val.type=type;
        val.value=value;
        val.ts=System.currentTimeMillis();
        sensorValues.put(key, val);
        
        if (sensorsAussen.contains(key))
        {
            Float minValue=null;
            for (String k: sensorsAussen)
            {
                Value v=getValue(k,false);
                if (v!=null && (minValue==null || minValue>v.value)) minValue=v.value;
            }
            if (minValue!=null) setSensorValue(SensorType.TEMPERATURE, "aussenMin", minValue, 0);
        }
        
    }
    
    protected static class Value
    {
        SensorType type;
        float value;
        long ts;
        boolean outdated;
    }
    
    protected String getFormattedValue(String key)
    {
        Value val=getValue(key,true);
        
        if (val==null) return "---";
        
        String result;
        
        if (val.type==SensorType.TEMPERATURE||val.type==SensorType.REMOTE_SWITCH_MIN_TEMP)
        {
            NumberFormat nf=NumberFormat.getInstance(Locale.GERMANY);
            int digits=(val.type==SensorType.REMOTE_SWITCH_MIN_TEMP)?0:1;
            nf.setMinimumFractionDigits(digits);
            nf.setMaximumFractionDigits(digits);
            result=nf.format(val.value)+" °C";
        }
        else if (val.type==SensorType.REMOTE_SWITCH_OUT)
        {
        	result=(val.value==0)?"OFF":"ON";
        }
        else if (val.type==SensorType.BRIGHTNESS)
        {
            NumberFormat nf=NumberFormat.getIntegerInstance(Locale.GERMANY);
            nf.setGroupingUsed(false);
            result=nf.format(val.value);
        }
        else
        {
	        NumberFormat nf=NumberFormat.getInstance(Locale.GERMANY);
	        nf.setMaximumFractionDigits(2);
	        result=nf.format(val.value);
        }
        if (val.outdated) result="("+result+")";
        return result;
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
            writer.println("<item><title>Pool</title><description>"+getFormattedValue("ds18s20-57728")+"</description></item>");
            writer.println("<item><title>Schuppen</title><description>"+getFormattedValue("ds18s20-8675")+"</description></item>");
            writer.println("<item><title>Lab</title><description>"+getFormattedValue("ds18s20-22317")+"</description></item>");
            writer.println("<item><title>Server</title><description>"+getFormattedValue("ds18s20-43501")+"</description></item>");
            writer.println("<item><title>Helligkeit</title><description>"+getFormattedValue("b1")+"</description></item>");
            writer.println("</channel>");
            writer.println("</rss>");
            return;
        }
                
        super.doGet(req, resp);
    }
}
