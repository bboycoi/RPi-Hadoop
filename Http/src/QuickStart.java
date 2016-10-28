/**
 * Created by nomad on Sep/28/16.
 */

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import org.apache.http.HttpEntity;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.util.EntityUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;


public class QuickStart {
	
	public static final String APPS_URL = "http://192.168.1.1:8088/ws/v1/cluster/apps";
	public static final String JOBS_URL1 = "http://192.168.1.1:8088/proxy/";
	public static final String JOBS_URL2 = "/ws/v1/mapreduce/jobs/";
	private static List<TaskAttempt> attempts = new ArrayList<TaskAttempt>();
	private static List<Task> tasks = new ArrayList<Task>();
	public static boolean isJSONValid(String str) {
	    try {
	        new JSONObject(str);
	    } catch (JSONException ex) {
	        // e.g. in case JSONArray is valid as well...
	        try {
	            new JSONArray(str);
	        } catch (JSONException ex1) {
	            return false;
	        }
	    }
	    return true;
	}
	
	public static String httpRequest(String url) throws Exception{
		String inputLine ;
		StringBuffer buffer = new StringBuffer();
        CloseableHttpClient httpclient = HttpClients.createDefault();
        try {
            HttpGet httpGet = new HttpGet(url);
            CloseableHttpResponse response1 = httpclient.execute(httpGet);
            try {
                //System.out.println(response1.getStatusLine());
                HttpEntity entity1 = response1.getEntity();
                BufferedReader br = new BufferedReader(new InputStreamReader(entity1.getContent()));
                while ((inputLine = br.readLine()) != null) {
                	buffer.append(inputLine + "\r\n");
                }
                br.close();
                EntityUtils.consume(entity1);
                return buffer.toString();
            }finally {response1.close();}
        } finally {httpclient.close();}
	}

	public static void getTaskIds(String appId, String jobId) throws Exception{
        //GET 	Task ID
        //System.out.println(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/");
        String buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/");
        if (isJSONValid(buffer)){
        	System.out.println(buffer);
        	JSONPaser jsonTasks = new JSONPaser(buffer);
	        //List<Task> tasks = new ArrayList<Task>();
	        List<Task> tasks1 = jsonTasks.tasksPasser();
	        for(Iterator<Task> i = tasks1.iterator(); i.hasNext();){
	        	Task task = i.next();
	        	//if(task.getState().equals("NEW")||task.getState().equals("SCHEDULED")||task.getState().equals("RUNNING")){
	        	tasks.add(task);
	        	//if(task.getType().equals("REDUCE")){
		        	buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/" +task.getId() + "/attempts");
		            buffer = buffer.replace("\"type\":\"reduceTaskAttemptInfo\",", "");
		            //buffer = buffer.replace("\"type\":\"mapTaskAttemptInfo\",", "");
		            if (isJSONValid(buffer)){
		            	//System.out.println(buffer);
		            	JSONPaser jsonAttempts = new JSONPaser(buffer);
		    	        //List<TaskAttempt> attempts = new ArrayList<TaskAttempt>();
		    	        List<TaskAttempt> attempts1 = jsonAttempts.taskAttemptsPasser();
		    	        for(Iterator<TaskAttempt> j = attempts1.iterator(); j.hasNext();){
		    	        	//TaskAttempt ta = j.next();
		    	        	//if(ta.getState().equals("NEW")||task.getState().equals("SCHEDULED")||task.getState().equals("RUNNING")){
		    	        	//if (ta.getType().equals("REDUCE")){	
		    	        	attempts.add(j.next());
		    	        	//}
		    	        }
		            }
	        	//}
	        }
        }
	} 
	
    public static void main(String[] args) throws IOException  {

    	String buffer, jobId, nodeId,taskAttmpId, appId;	
    	int fileSize = 1024;
    	int clusterSize = 5;
    	int blockSize = 20;	
    	int cpus = 2;
    	buffer="";
    	appId = "";
        Context context = ZMQ.context(1);
        Socket publisher = context.socket(ZMQ.PUB);
        publisher.bind("tcp://*:9999");
    	
    	//Get App ID
    	//while (buffer==null){
	    	try {
				buffer = httpRequest(APPS_URL);
		        if (isJSONValid(buffer)){
		        	System.out.println(buffer);
			        JSONPaser jsonApps = new JSONPaser(buffer);
			        List<Application> apps = new ArrayList<Application>();
			        apps = jsonApps.appsPasser();
			        for(Iterator<Application> i = apps.iterator(); i.hasNext(); ){
			        	Application app = i.next();
			        	if(app.getState().equals("RUNNING")||app.getState().equals("NEW")||app.getState().equals("ACCEPTED")){
			        		appId = app.getId();
			        		System.out.println(appId);
			        	}
			        }
		        }
			} catch (Exception e1) {
				e1.printStackTrace();
			}
    //	}
    	
    	buffer="";
    	jobId = "";
        //get JOB ID
        System.out.println(JOBS_URL1 + appId + JOBS_URL2);
      //  while (buffer==null){
	        try {
				buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2);
		        if (isJSONValid(buffer)){
		        	System.out.println(buffer);
		        	JSONPaser jsonJobs = new JSONPaser(buffer);
			        List<Job> jobs = new ArrayList<Job>();
			        jobs = jsonJobs.jobsPasser();
			        for(Iterator<Job> i = jobs.iterator(); i.hasNext(); ){
			        	Job job = i.next();
			        	if(job.getState().equals("NEW")||job.getState().equals("INITED")||job.getState().equals("RUNNING")){
			        		jobId = job.getId();
			        		System.out.println(jobId);
			        	}
			        }
		        }
			} catch (Exception e1) {
				e1.printStackTrace();
			}
       // }
        
        buffer = "";
        taskAttmpId = "";
        nodeId = "";
        
    	try {
			getTaskIds(appId,jobId);
		} catch (Exception e) {
			e.printStackTrace();
		}
        
        for(Iterator<TaskAttempt> i = attempts.iterator(); i.hasNext();){
        	TaskAttempt ta = i.next();
        	if (ta.getType().equals("REDUCE")){	
	        	taskAttmpId = ta.getId();
	        	System.out.println(taskAttmpId);
	        	nodeId = ta.getNodeHttpAddress();
	        	System.out.println(nodeId);
	        	System.out.println(ta.getState());
        	}
        }
        
        int rNodeId = 0;
        if(nodeId!=null){
            rNodeId = Integer.parseInt(nodeId.substring(4,nodeId.indexOf(":")));
        	System.out.println(rNodeId);
        }
    	
    	int begin = fileSize/(blockSize*clusterSize) - cpus;
    	begin = clusterSize * cpus + (clusterSize - rNodeId) * begin;
    	int end = begin + (fileSize/(blockSize*clusterSize) - cpus) - 1; 
    	System.out.println("begin="+begin+" end="+end);
    	List<String> montTaskIDs = new ArrayList<String>();
        for(Iterator<Task> i = tasks.iterator(); i.hasNext();){
        	Task task = i.next();
        	if (task.getType().equals("MAP")){	
        		String taskId = task.getId();
        		int number = Integer.parseInt(taskId.substring(taskId.length() - 5));
        		if (number >= begin && number <= end){
        			montTaskIDs.add(taskId);
        			System.out.println(taskId);
        		}
        	}
        }

        int epoch = 8000;
		int tAlloc[] = {0, 2000, 3500, 5000, 6500, 8000};
		while (nodeId!=null){
			boolean changed = false;
			int arrToSend [] = new int [6];
			if (!changed){
	        	for (int k = 1; k<=clusterSize; k++){
	        		if (k >= rNodeId) 
	        			arrToSend[k] = tAlloc[k] - (200*(clusterSize-1));
	        		else arrToSend[k] = tAlloc[k];
	        	}
	        	//send arrToSend[]
	        	System.out.println(Arrays.toString(arrToSend));
				String str = Arrays.toString(arrToSend);
				str =str.replace("]", "");
				str =str.replace("[", "");
				str =str.replace(" ", "");
				publisher.send (str);
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
		
		/*while (nodeId!=null){
			boolean changed = false;
			int arrToSend [] = new int [5];
			try {
				for(Iterator<String> i = montTaskIDs.iterator(); i.hasNext();){
		    		String taskId = i.next();
					buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/" + taskId + "/attempts");
					buffer = buffer.replace("\"type\":\"reduceTaskAttemptInfo\",", "");
			        buffer = buffer.replace("\"type\":\"mapTaskAttemptInfo\",", "");
			        if (isJSONValid(buffer)){
			        	//System.out.println(buffer);
			        	JSONPaser jsonAttempts1 = new JSONPaser(buffer);
				        List<TaskAttempt> attempts2 = new ArrayList<TaskAttempt>();
				        attempts2 = jsonAttempts1.taskAttemptsPasser();
				        for(Iterator<TaskAttempt> j = attempts2.iterator(); j.hasNext();){
				        	TaskAttempt ta = j.next();
				        	String rId = ta.getNodeHttpAddress();
				        	String jobState = ta.getState();
				        	if(rId!=null && !rId.equals(nodeId)){
				        		if(!jobState.equals("SUCCEEDED")||!jobState.equals("KILLED")){
						        	System.out.println(taskId);		
						        	System.out.println(rId);
						        	System.out.println(jobState);
						        	changed = true;
				        		}
				        	}
				        }
			        }
			        Thread.sleep(200);
				}
				if (!changed){
		        	for (int k = 1; k<=clusterSize; k++){
		        		if (k >= rNodeId) 
		        			arrToSend[k] = tAlloc[k] - (100*(clusterSize-1));
		        		else arrToSend[k] = tAlloc[k];
		        	}
		        	//send arrToSend[]
		        	System.out.println(Arrays.toString(arrToSend));
					String str = Arrays.toString(arrToSend);
					str =str.replace("]", "");
					str =str.replace("[", "");
					str =str.replace(" ", "");
					publisher.send (str);
				}else {
					System.out.println(Arrays.toString(tAlloc));
					//send tAlloc[]
					String str = Arrays.toString(tAlloc);
					str =str.replace("]", "");
					str =str.replace("[", "");
					str =str.replace(" ", "");
					publisher.send (str);
				}
				Thread.sleep(2000);
			} catch (Exception e) {e.printStackTrace();}
		}
*/
        
  /*  	try {
			socket.close();
	    	listener.close();
		} catch (IOException e) {
			e.printStackTrace();
		}*/



      /*  //GET 	Task ID
        System.out.println(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/");
        buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/");
        if (isJSONValid(buffer)){
        	System.out.println(buffer);
        	JSONPaser jsonTasks = new JSONPaser(buffer);
	        List<Task> tasks = new ArrayList<Task>();
	        tasks = jsonTasks.tasksPasser();
	        for(Iterator<Task> i = tasks.iterator(); i.hasNext();){
	        	Task task = i.next();
	        	//if(task.getState().equals("NEW")||task.getState().equals("SCHEDULED")||task.getState().equals("RUNNING")){
	        	if (task.getType().equals("REDUCE")){	
	        		taskId = task.getId();
	        		System.out.println(taskId);
	        	}
	        }
        }*/
        
        /*
        taskAttmpId = "";
        nodeId = "";
        //GET NODE ID
        System.out.println(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/" + rTaskId + "/attempts");
        buffer = httpRequest(JOBS_URL1 + appId + JOBS_URL2 + jobId + "/tasks/" + rTaskId + "/attempts");
        buffer = buffer.replace("\"type\":\"reduceTaskAttemptInfo\",", "");
        buffer = buffer.replace("\"type\":\"mapTaskAttemptInfo\",", "");
        if (isJSONValid(buffer)){
        	System.out.println(buffer);
        	JSONPaser jsonAttempts = new JSONPaser(buffer);
	        List<TaskAttempt> attempts = new ArrayList<TaskAttempt>();
	        attempts = jsonAttempts.taskAttemptsPasser();
	        for(Iterator<TaskAttempt> i = attempts.iterator(); i.hasNext();){
	        	TaskAttempt ta = i.next();
	        	//if(ta.getState().equals("NEW")||task.getState().equals("SCHEDULED")||task.getState().equals("RUNNING")){
	        	//if (ta.getType().equals("REDUCE")){	
	        	taskAttmpId = ta.getId();
	        	System.out.println(taskAttmpId);
	        	nodeId = ta.getNodeHttpAddress();
	        	System.out.println(nodeId);
	        	System.out.println(ta.getState());
	        	//}
	        }
        }*/
/*        
        containerId = "";
        remoteNode = "";
        //GET NODE ID
        System.out.println("http://"+ nodeId +"/ws/v1/node/containers");
        buffer = httpRequest("http://"+ nodeId +"/ws/v1/node/containers");
        //buffer = buffer.replace("\"type\":\"reduceTaskAttemptInfo\",", "");
        //buffer = buffer.replace("\"type\":\"mapTaskAttemptInfo\",", "");
       // if (isJSONValid(buffer)){
        	System.out.println(buffer);
        	JSONPaser jsonCons = new JSONPaser(buffer);
	        List<Container> cons = new ArrayList<Container>();
	        cons = jsonCons.containersPasser();
	        for(Iterator<Container> i = cons.iterator(); i.hasNext();){
	        	Container con = i.next();
	        	//if(ta.getState().equals("NEW")||task.getState().equals("SCHEDULED")||task.getState().equals("RUNNING")){
	        	//if (ta.getType().equals("REDUCE")){	
	        	containerId = con.getId();
	        	System.out.println(containerId);
	        	remoteNode = con.getNodeId();
	        	System.out.println(remoteNode);
	        	System.out.println(con.getState());
	        	System.out.println(con.getContainerLogsLink());
	        	//}
	        }
       // }*/
    }

}
