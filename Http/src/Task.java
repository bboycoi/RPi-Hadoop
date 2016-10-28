
public class Task {
	
	private String  type, id, state, successfulAttempt ;
	private long startTime, finishTime, elapsedTime ;
	private float progress;
	
	public void setType (String type){this.type = type;}
	
	public String getType (){return this.type;}
	
	public void setId (String id){this.id = id;}
	
	public String getId (){return this.id;}
	
	public void setState (String state){this.state = state;}
	
	public String getState (){return this.state;}
	
	public void setSuccessfulAttempt (String successfulAttempt){this.successfulAttempt = successfulAttempt;}
	
	public String getSuccessfulAttempt (){return this.successfulAttempt;}
	
	public void setStartTime (long startTime){this.startTime = startTime;}
	
	public long getStartTime (){return this.startTime;}
	
	public void setFinishTime (long finishTime){this.finishTime = finishTime;}
	
	public long getFinishTime (){return this.finishTime;}
	
	public void setElapsedTime (long elapsedTime){this.elapsedTime = elapsedTime;}
	
	public long getElapsedTime (){return this.elapsedTime;}
	
	public void setProgress (float progress){this.progress = progress;}
	
	public float getProgress (){return this.progress;}
}
