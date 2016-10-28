/**
 * Created by nomad on Sep/28/16.
 */

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
 


public class JSONPaser {

    private static String data;
    
    public JSONPaser (String data){
    	JSONPaser.data = data;
    }

    public List<TaskAttempt> taskAttemptsPasser() throws JSONException {

    	List<TaskAttempt> taskAttempts = new ArrayList<TaskAttempt>();
        //  Create  JSONObject from the data
        try {
            JSONObject jObj = new JSONObject(data);
            JSONObject jSONTasks = getObject("taskAttempts", jObj);
            //  Start extracting the info
            JSONArray jArr = jSONTasks.getJSONArray("taskAttempt");
            for (int i = 0; i < jArr.length(); i++) {
                JSONObject jSONTaskAttempt = jArr.getJSONObject(i);
                TaskAttempt ta = new TaskAttempt();
                if (jSONTaskAttempt.has("nodeHttpAddress"))
                	ta.setNodeHttpAddress(getString("nodeHttpAddress",jSONTaskAttempt));
                if (jSONTaskAttempt.has("assignedContainerId"))
                	ta.setAssignedContainerId(getString("assignedContainerId",jSONTaskAttempt));
                ta.setState(getString("state",jSONTaskAttempt));
                ta.setType(getString("type",jSONTaskAttempt));
                //ta.setDiagnostics(getString("diagnostics ",jSONTaskAttempt));
                ta.setId(getString("id",jSONTaskAttempt));
                ta.setProgress(getFloat("progress",jSONTaskAttempt));
                taskAttempts.add(ta);
                }
            return taskAttempts;
            }
        catch (NullPointerException e) {
            e.printStackTrace();
        }
        return null;
    }
    
    public List<Task> tasksPasser() throws JSONException {

    	List<Task> tasks = new ArrayList<Task>();

        //  Create  JSONObject from the data
        try {
            JSONObject jObj = new JSONObject(data);
            JSONObject jSONTasks = getObject("tasks", jObj);
            //  Start extracting the info
            JSONArray jArr = jSONTasks.getJSONArray("task");
            for (int i = 0; i < jArr.length(); i++) {
                JSONObject jSONTask = jArr.getJSONObject(i);
                Task task = new Task();
                task.setId(getString("id",jSONTask));               
                task.setState(getString("state",jSONTask));
                task.setType(getString("type",jSONTask));
                task.setProgress(getFloat("progress",jSONTask));
                tasks.add(task);
                }
            return tasks;
            }
        catch (NullPointerException e) {
            e.printStackTrace();
        }
        return null;
    }
    
    public List<Container> containersPasser() throws JSONException {

    	List<Container> cons = new ArrayList<Container>();

        //  Create  JSONObject from the data
        try {
            JSONObject jObj = new JSONObject(data);
            JSONObject jSONCons = getObject("containers", jObj);
            //  Start extracting the info
            JSONArray jArr = jSONCons.getJSONArray("container");
            for (int i = 0; i < jArr.length(); i++) {
                JSONObject jSONCon = jArr.getJSONObject(i);
                Container con = new Container();
                con.setId(getString("id",jSONCon));               
                con.setState(getString("state",jSONCon));
                con.setNodeId(getString("nodeId",jSONCon));
                cons.add(con);
                }
            return cons;
            }
        catch (NullPointerException e) {
            e.printStackTrace();
        }
        return null;
    }
    
    public List<Application> appsPasser() throws JSONException {

    	List<Application> apps = new ArrayList<Application>();

        //  Create  JSONObject from the data
        try {
            JSONObject jObj = new JSONObject(data);
            JSONObject jSONApps = getObject("apps", jObj);
            //  Start extracting the info
            JSONArray jArr = jSONApps.getJSONArray("app");
            for (int i = 0; i < jArr.length(); i++) {
                JSONObject jSONapp = jArr.getJSONObject(i);
                Application app = new Application();
                app.setId(getString("id",jSONapp));               
                app.setState(getString("state",jSONapp));
                app.setName(getString("name",jSONapp));
                apps.add(app);
                }
            return apps;
            }
        catch (NullPointerException e) {
            e.printStackTrace();
        }
        return null;
    }
    
    public List<Job> jobsPasser() throws JSONException {

    	List<Job> jobs = new ArrayList<Job>();

        //  Create  JSONObject from the data
        try {
            JSONObject jObj = new JSONObject(data);
            JSONObject jSONJobs = getObject("jobs", jObj);
            //  Start extracting the info
            JSONArray jArr = jSONJobs.getJSONArray("job");
            for (int i = 0; i < jArr.length(); i++) {
                JSONObject jSONJob = jArr.getJSONObject(i);
                Job job = new Job();
                job.setId(getString("id",jSONJob));               
                job.setState(getString("state",jSONJob));
                job.setName(getString("name",jSONJob));
                jobs.add(job);
                }
            return jobs;
            }
        catch (NullPointerException e) {
            e.printStackTrace();
        }
        return null;
    }

    private static JSONObject getObject(String tagName, JSONObject jObj)  throws JSONException {
        JSONObject subObj = jObj.getJSONObject(tagName);
        return subObj;
    }

    private static String getString(String tagName, JSONObject jObj) throws JSONException {
        return jObj.getString(tagName);
    }

    private static float  getFloat(String tagName, JSONObject jObj) throws JSONException {
        return (float) jObj.getDouble(tagName);
    }

    private static int  getInt(String tagName, JSONObject jObj) throws JSONException {
        return jObj.getInt(tagName);
    }


}
