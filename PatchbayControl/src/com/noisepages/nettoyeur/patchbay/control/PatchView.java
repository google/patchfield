package com.noisepages.nettoyeur.patchbay.control;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import android.app.PendingIntent;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Typeface;
import android.os.RemoteException;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.view.MotionEvent;
import android.view.View;

import com.noisepages.nettoyeur.patchbay.IPatchbayService;

public final class PatchView extends View {

	private IPatchbayService patchbay;
	private final List<String> modules = new ArrayList<String>();
	private final Map<String, Integer> inputs = new HashMap<String, Integer>();
	private final Map<String, Integer> outputs = new HashMap<String, Integer>();
	private final Map<String, PendingIntent> intents = new HashMap<String, PendingIntent>();
	private final Map<Pair<String, Integer>, List<Pair<String, Integer>>> connections =
			new HashMap<Pair<String,Integer>, List<Pair<String,Integer>>>();
	
	private final Path path = new Path();
	private final Paint linePaint = new Paint();
	private final Paint drawPaint = new Paint();
	private final Paint labelPaint = new Paint();
	private final Paint portPaint = new Paint();

	private int width, height;
	private float x0, y0, x1 = -1, y1 = -1;
	private Pair<String, Integer> outputPort = null;
	
	public PatchView(Context context) {
		super(context);
		init();
	}

	public PatchView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	public PatchView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init();
	}

	private void init() {
		linePaint.setAntiAlias(true);
		linePaint.setColor(Color.BLACK);
		linePaint.setStrokeWidth(5);
		linePaint.setStyle(Paint.Style.STROKE);
		
		drawPaint.setAntiAlias(true);
		drawPaint.setColor(Color.BLUE);
		drawPaint.setStrokeWidth(5);
		drawPaint.setStyle(Paint.Style.STROKE);
		
		labelPaint.setAntiAlias(true);
		labelPaint.setColor(Color.BLACK);
		labelPaint.setTextAlign(Paint.Align.CENTER);
		labelPaint.setTypeface(Typeface.MONOSPACE);
		labelPaint.setTextSize(24);
		
		portPaint.setAntiAlias(true);
		portPaint.setColor(Color.BLACK);
		portPaint.setStyle(Paint.Style.FILL);
	}

	public void setPatchbay(IPatchbayService patchbay) {
		this.patchbay = patchbay;
	}
	
	public void addModule(String module, int inputChannels, int outputChannels, PendingIntent intent) {
		for (String u : modules) {
			int sinks = 0;
			try {
				sinks = patchbay.getInputChannels(u);
			} catch (RemoteException e) {
				e.printStackTrace();
				continue;
			}
			for (int i = 0; i < outputChannels; ++i) {
				for (int j = 0; j < sinks; ++j) {
					try {
						if (patchbay.isConnected(module, i, u, j)) {
							addConnection(module, i, u, j);
						}
					} catch (RemoteException e) {
						e.printStackTrace();
					}
				}
			}
			int sources = 0;
			try {
				sources = patchbay.getOutputChannels(u);
			} catch (RemoteException e) {
				e.printStackTrace();
				continue;
			}
			for (int i = 0; i < inputChannels; ++i) {
				for (int j = 0; j < sources; ++j) {
					try {
						if (patchbay.isConnected(u, j, module, i)) {
							addConnection(u, j, module, i);
						}
					} catch (RemoteException e) {
						e.printStackTrace();
					}
				}
			}
		}
		modules.add(module);
		inputs.put(module, inputChannels);
		outputs.put(module, outputChannels);
		intents.put(module, intent);
		invalidate();
	}
	
	public void deleteModule(String module) {
		for (Iterator<Entry<Pair<String, Integer>, List<Pair<String, Integer>>>> iter = connections.entrySet().iterator(); iter.hasNext();) {
			Entry<Pair<String, Integer>, List<Pair<String, Integer>>> entry = iter.next();
			if (entry.getKey().first.equals(module)) {
				iter.remove();
			} else {
				for (Iterator<Pair<String, Integer>> iter2 = entry.getValue().iterator(); iter2.hasNext();) {
					Pair<String, Integer> q = iter2.next();
					if (q.first.equals(module)) {
						iter2.remove();
					}
				}
			}
		}
		modules.remove(module);
		inputs.remove(module);
		outputs.remove(module);
		intents.remove(module);
		invalidate();
	}
	
	public void addConnection(String source, int sourcePort, String sink, int sinkPort) {
		Pair<String, Integer> a = new Pair<String, Integer>(source, sourcePort);
		List<Pair<String, Integer>> c = connections.get(a);
		if (c == null) {
			c = new ArrayList<Pair<String, Integer>>();
			connections.put(a, c);
		}
		Pair<String, Integer> b = new Pair<String, Integer>(sink, sinkPort);
		if (!c.contains(b)) {
			c.add(b);
		}
		invalidate();
	}

	public void removeConnection(String source, int sourcePort, String sink,
			int sinkPort) {
		Pair<String, Integer> a = new Pair<String, Integer>(source, sourcePort);
		List<Pair<String, Integer>> c = connections.get(a);
		if (c == null) {
			return;
		}
		Pair<String, Integer> b = new Pair<String, Integer>(sink, sinkPort);
		c.remove(b);
		invalidate();
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		width = w;
		height = h;
	}

	private Pair<Float, Float> getInputLabelLocation(String module) {
		int i = modules.indexOf(module);
		float w = width / (modules.size() + 1);
		float x = (i + 1) * w;
		float y = 9 * height / 10;
		return new Pair<Float, Float>(x, y);
	}
	
	private Pair<Float, Float> getOutputLabelLocation(String module) {
		int i = modules.indexOf(module);
		float w = width / (modules.size() + 1);
		float x = (i + 1) * w;
		float y = height / 10;
		return new Pair<Float, Float>(x, y);
	}
	
	private Pair<Float, Float> getOutputPortLocation(String module, int port) {
		int i = modules.indexOf(module);
		float w = width / (modules.size() + 1);
		float xm = (i + 1) * w;
		int n = outputs.get(module);
		float x = xm;
		if (n > 1) {
			float w2 = w * 0.5f;
			x = xm - w2 / 2 + ((float) port / (n - 1)) * w2;
		}
		float y = 2 * height / 10;
		return new Pair<Float, Float>(x, y);
	}
	
	private Pair<Float, Float> getInputPortLocation(String module, int port) {
		int i = modules.indexOf(module);
		float w = width / (modules.size() + 1);
		float xm = (i + 1) * w;
		int n = inputs.get(module);
		float x = xm;
		if (n > 1) {
			float w2 = w * 0.5f;
			x = xm - w2 / 2 + ((float) port / (n - 1)) * w2;
		}
		float y = 8 * height / 10;
		return new Pair<Float, Float>(x, y);
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		if (x1 >= 0 && y1 >= 0) {
			path.reset();
			path.moveTo(x0, y0);
			path.lineTo(x1, y1);
			canvas.drawPath(path, drawPaint);
		}
		for (String module : modules) {
			if (inputs.get(module) > 0) {
				Pair<Float, Float> p = getInputLabelLocation(module);
				canvas.drawText(module, p.first, p.second, labelPaint);
				for (int i = 0; i < inputs.get(module); ++i) {
					Pair<Float, Float> p0 = getInputPortLocation(module, i);
					canvas.drawCircle(p0.first, p0.second, 15, portPaint);
				}
			}
			if (outputs.get(module) > 0) {
				Pair<Float, Float> p = getOutputLabelLocation(module);
				canvas.drawText(module, p.first, p.second, labelPaint);
				for (int i = 0; i < outputs.get(module); ++i) {
					Pair<Float, Float> p0 = getOutputPortLocation(module, i);
					canvas.drawCircle(p0.first, p0.second, 15, portPaint);
					Pair<String, Integer> port = new Pair<String, Integer>(module, i);
					if (connections.containsKey(port)) {
						for (Pair<String, Integer> sink : connections.get(port)) {
							Pair<Float, Float> p1 = getInputPortLocation(sink.first, sink.second);
							path.reset();
							path.moveTo(p0.first, p0.second);
							path.lineTo(p1.first, p1.second);
							canvas.drawPath(path, linePaint);
						}
					}
				}
			}
		}
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		float x = event.getX();
		float y = event.getY();
		switch (event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			outputPort = null;
			for (String module : modules) {
				for (int i = 0; i < outputs.get(module); ++i)  {
					Pair<Float, Float> p = getOutputPortLocation(module, i);
					float dx = p.first - x;
					float dy = p.second - y;
					if (dx * dx + dy * dy < 900) {
						x0 = p.first;
						y0 = p.second;
						outputPort = new Pair<String, Integer>(module, i);
					}
				}
			}
			break;
		case MotionEvent.ACTION_MOVE:
			if (outputPort != null) {
				x1 = x;
				y1 = y;
			}
			break;
		case MotionEvent.ACTION_UP:
			if (outputPort != null) {
				for (String module : modules) {
					for (int i = 0; i < inputs.get(module); ++i)  {
						Pair<Float, Float> p = getInputPortLocation(module, i);
						float dx = p.first - x;
						float dy = p.second - y;
						if (dx * dx + dy * dy < 900) {
							try {
								if (patchbay.isConnected(outputPort.first, outputPort.second, module, i)) {
									patchbay.disconnectModules(outputPort.first, outputPort.second, module, i);
								} else {
									patchbay.connectModules(outputPort.first, outputPort.second, module, i);
								}
							} catch (RemoteException e) {
								e.printStackTrace();
							}
						}
					}
				}
			}
			x1 = -1;
			y1 = -1;
			outputPort = null;
			break;
		default:
			break;
		}
		invalidate();
		return true;
	}
}
