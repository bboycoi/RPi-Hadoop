
public class TaskAttempt {
	
	private String nodeHttpAddress, type, id, assignedContainerId,rack, state,diagnostics;
	private long startTime, finishTime, mergeFinishTime, elapsedShuffleTime, elapsedReduceTime;
	private long elapsedTime, shuffleFinishTime, elapsedMergeTime; 
	private float progress;
	
	public void setNodeHttpAddress (String nodeHttpAddress){this.nodeHttpAddress = nodeHttpAddress;}
	
	public String getNodeHttpAddress (){ return this.nodeHttpAddress;}
	
	public void setType (String type){this.type = type;}
	
	public String getType (){return this.type;}
	
	public void setId (String id){this.id = id;}
	
	public String getId (){return this.id;}
	
	public void setDiagnostics (String diagnostics){this.diagnostics = diagnostics;}
	
	public String getDiagnostics (){return this.diagnostics;}
	
	public void setAssignedContainerId (String assignedContainerId){this.assignedContainerId = assignedContainerId;}
	
	public String getAssignedContainerId (){return this.assignedContainerId;}
	
	public void setRack (String rack){this.rack = rack;}
	
	public String getRack (){return this.rack;}
	
	public void setState (String state){this.state = state;}
	
	public String getState (){return this.state;}
	
	public void setStartTime (long startTime){this.startTime = startTime;}
	
	public long getStartTime (){return this.startTime;}
	
	public void setFinishTime (long finishTime){this.finishTime = finishTime;}
	
	public long getFinishTime (){return this.finishTime;}
	
	public void setMergeFinishTime (long mergeFinishTime){this.mergeFinishTime = mergeFinishTime;}
	
	public long getMergeFinishTime (){return this.mergeFinishTime;}
	
	public void setElapsedShuffleTime (long elapsedShuffleTime){this.elapsedShuffleTime = elapsedShuffleTime;}
	
	public long getElapsedShuffleTime (){return this.elapsedShuffleTime;}
	
	public void setElapsedTime (long elapsedTime){this.elapsedTime = elapsedTime;}
	
	public long getElapsedTime (){return this.elapsedTime;}
	
	public void setShuffleFinishTime (long shuffleFinishTime){this.shuffleFinishTime = shuffleFinishTime;}
	
	public long getShuffleFinishTime (){return this.shuffleFinishTime;}
	
	public void setElapsedMergeTime (long elapsedMergeTime){this.elapsedMergeTime = elapsedMergeTime;}
	
	public long getElapsedMergeTime (){return this.elapsedMergeTime;}
	
	public void setElapsedReduceTime (long elapsedReduceTime){this.elapsedReduceTime = elapsedReduceTime;}
	
	public long getElapsedReduceTime (){return this.elapsedReduceTime;}
	
	public void setProgress (float progress){this.progress = progress;}
	
	public float getProgress (){return this.progress;}
}
