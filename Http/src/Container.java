
public class Container {
	private String id, state, nodeId, containerLogsLink, user, diagnostics;
	private int exitCode;
	private long totalMemoryNeededMB, totalVCoresNeeded ;
	
	public void setId (String id){this.id = id;}
	
	public String getId (){return this.id;}
	
	public void setNodeId(String nodeId){this.nodeId = nodeId;}
	
	public String getNodeId (){return this.nodeId;}
	
	public void setState (String state){this.state = state;}
	
	public String getState (){return this.state;}
	
	public void setDiagnostics (String diagnostics){this.diagnostics = diagnostics;}
	
	public String getDiagnostics (){return this.diagnostics;}
	
	public void setContainerLogsLink(String containerLogsLink){this.containerLogsLink = containerLogsLink;}
	
	public String getContainerLogsLink (){return this.containerLogsLink;}
	
	public void setUser (String user){this.user = user;}
	
	public String getUser (){return this.user;}
	
	public void setExitCode (int exitCode){this.exitCode = exitCode;}
	
	public int getExitCode (){return this.exitCode;}
	
	public void setTotalMemoryNeededMB (long totalMemoryNeededMB){this.totalMemoryNeededMB = totalMemoryNeededMB;}
	
	public long getTotalMemoryNeededMB (){return this.totalMemoryNeededMB;}
	
	public void setTotalVCoresNeeded (long totalVCoresNeeded){this.totalVCoresNeeded = totalVCoresNeeded;}
	
	public long getTotalVCoresNeeded (){return this.totalVCoresNeeded;}
}
